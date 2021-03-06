/*
 *  ConnectedClient.cpp
 */

#include "ConnectedClient.h"

#include <cmath>
#include "RaptorDefs.h"
#include "RaptorServer.h"
#include "PacketBuffer.h"
#include "Rand.h"
#include "Num.h"
#include "RaptorServer.h"


ConnectedClient::ConnectedClient( TCPsocket socket, bool use_out_thread, double net_rate, int8_t precision )
{
	Connected = false;
	
	Socket = socket;
	IP = 0;
	Port = 0;
	Synchronized = false;
	NetRate = net_rate;
	PingRate = 4.;
	Precision = precision;
	BytesSent = 0;
	BytesReceived = 0;
	InThread = NULL;
	OutThread = NULL;
	UseOutThread = use_out_thread;
	
	Connected = true;
	
	// Start the listener thread.
	if( !( InThread = SDL_CreateThread( ConnectedClientInThread, this ) ) )
	{
		fprintf( stderr, "ConnectedClient::ConnectedClient: SDL_CreateThread(ConnectedClientInThread): %s\n", SDLNet_GetError() );
		Connected = false;
	}
	
	if( UseOutThread )
	{
		// Start the sender thread.
		if( !( OutThread = SDL_CreateThread( ConnectedClientOutThread, this ) ) )
		{
			fprintf( stderr, "ConnectedClient::ConnectedClient: SDL_CreateThread(ConnectedClientOutThread): %s\n", SDLNet_GetError() );
			UseOutThread = false;
		}
	}
}


ConnectedClient::~ConnectedClient()
{
	Disconnect();
	
	// Sleep until the other threads have finished (max 2 sec).
	Clock wait_for_thread;
	while( (InThread || OutThread) && (wait_for_thread.ElapsedSeconds() < 2.) )
		SDL_Delay( 1 );
	
	// If either thread didn't finish, kill it.
	if( InThread )
	{
		SDL_KillThread( InThread );
		InThread = NULL;
	}
	if( OutThread )
	{
		SDL_KillThread( OutThread );
		OutThread = NULL;
	}
	
	Cleanup();
}


void ConnectedClient::DisconnectNice( const char *message )
{
	if( Connected )
	{
		Packet packet( Raptor::Packet::DISCONNECT );
		
		if( message )
			packet.AddString( message );
		else
			packet.AddString( "" );
		
		Send( &packet );
	}
	
	Disconnect();
}


void ConnectedClient::Disconnect( void )
{
	Synchronized = false;
	
	if( Connected )
	{
		char cstr[ 1024 ] = "";
		snprintf( cstr, 1024, "Client dropped: %i.%i.%i.%i:%i", (IP & 0xFF000000) >> 24, (IP & 0x00FF0000) >> 16, (IP & 0x0000FF00) >> 8, IP & 0x000000FF, Port );
		Raptor::Server->ConsolePrint( cstr );
		
		Connected = false;
		Raptor::Server->DroppedClient( this );
	}
	
	// Handle cleanup later, after the threads are finished.
}


void ConnectedClient::Cleanup( void )
{
	if( Socket )
	{
		SDLNet_TCP_Close( Socket );
		Socket = NULL;
	}
	
	while( ! InBuffer.empty() )
	{
		Packet *packet = InBuffer.front();
		InBuffer.pop();
		delete packet;
	}
	
	while( ! OutBuffer.empty() )
	{
		Packet *packet = OutBuffer.front();
		OutBuffer.pop();
		delete( packet );
	}
}


void ConnectedClient::ProcessIn( void )
{
	if( ! Connected )
		return;
	
	if( ! InLock.Lock() )
		fprintf( stderr, "ConnectedClient::ProcessIn: InLock.Lock: %s\n", SDL_GetError() );
	
	// While locked, process all packets on the input buffer.
	while( ! InBuffer.empty() )
	{
		Packet *packet = InBuffer.front();
		ProcessPacket( packet );
		delete packet;
		InBuffer.pop();
	}
	
	if( ! InLock.Unlock() )
		fprintf( stderr, "ConnectedClient::ProcessIn: InLock.Unlock: %s\n", SDL_GetError() );
}


void ConnectedClient::ProcessTop( void )
{
	if( ! Connected )
		return;
	
	if( ! InLock.Lock() )
		fprintf( stderr, "ConnectedClient::ProcessTop: InLock.Lock: %s\n", SDL_GetError() );
	
	// While locked, process the oldest packet on the input buffer.
	if( ! InBuffer.empty() )
	{
		Packet *packet = InBuffer.front();
		InBuffer.pop();
		ProcessPacket( packet );
		delete packet;
	}
	
	if( ! InLock.Unlock() )
		fprintf( stderr, "ConnectedClient::ProcessTop: InLock.Unlock: %s\n", SDL_GetError() );
}


bool ConnectedClient::ProcessPacket( Packet *packet )
{
	packet->Rewind();
	PacketType type = packet->Type();
	
	if( Raptor::Server->ProcessPacket( packet, this ) )
		return true;
	
	else if( type == Raptor::Packet::PING )
	{
		uint8_t ping_id = packet->NextUChar();
		Packet pong( Raptor::Packet::PONG );
		pong.AddUChar( ping_id );
		Send( &pong );
	}
	
	else if( type == Raptor::Packet::PONG )
	{
		uint8_t ping_id = packet->NextUChar();
		std::map<uint8_t,Clock>::iterator ping_iter = SentPings.find( ping_id );
		if( ping_iter != SentPings.end() )
		{
			double ms = ping_iter->second.ElapsedMilliseconds();
			SentPings.erase( ping_iter );
			PingTimes.push_back( ms );
			while( PingTimes.size() > 120 )
				PingTimes.pop_front();
		}
	}
	
	else if( type == Raptor::Packet::PADDING )
	{
		// Always ignore padding packets.
		packet->Offset = packet->Size();
	}
	
	else if( type == Raptor::Packet::LOGIN )
	{
		std::string game = packet->NextString();
		std::string version = packet->NextString();
		std::string name = packet->NextString();
		std::string password = packet->NextString();
		
		if( game == Raptor::Server->Game )
		{
			if( version == Raptor::Server->Version )
				Login( name, password );
			else
				DisconnectNice( "Version mismatch.  Make sure all players have the latest build." );
		}
		else
			Disconnect();
	}
	
	else if( type == Raptor::Packet::DISCONNECT )
	{
		std::string message = packet->NextString();
		
		Disconnect();
	}
	
	else
		return false;
	
	return true;
}


void ConnectedClient::Login( std::string name, std::string password )
{
	PlayerID = 0;
	
	bool valid_login = Raptor::Server->ValidateLogin( name, password );
	if( valid_login )
	{
		Player *player = new Player();
		PlayerID = Raptor::Server->Data.AddPlayer( player );
		if( ! PlayerID )
			delete player;
	}
	
	if( PlayerID )
	{
		Raptor::Server->Data.Players[ PlayerID ]->Name = name;
		
		Packet accept( Raptor::Packet::LOGIN );
		accept.AddUShort( PlayerID );
		Send( &accept );
		
		Raptor::Server->AcceptedClient( this );
	}
	else
		Disconnect();
}


bool ConnectedClient::Send( Packet *packet )
{
	if( UseOutThread )
	{
		SendToOutBuffer( packet );
		return true;
	}
	else
		return SendNow( packet );
}


bool ConnectedClient::SendNow( Packet *packet )
{
	if( ! Connected )
		return false;
	
	if( SDLNet_TCP_Send( Socket, (void *) packet->Data, packet->Size() ) < (int) packet->Size() )
	{
		Disconnect();
		return false;
	}
	else
		BytesSent += packet->Size();
	
	return true;
}


void ConnectedClient::SendToOutBuffer( Packet *packet )
{
	if( ! Connected )
		return;
	
	// Make a copy of the outgoing packet.
	Packet *packet_copy = new Packet(packet);
	
	if( ! OutLock.Lock() )
		fprintf( stderr, "ConnectedClient::Send: OutLock.Lock: %s\n", SDL_GetError() );
	
	// While locked, add the packet copy to the outgoing buffer.
	OutBuffer.push( packet_copy );
	
	if( ! OutLock.Unlock() )
		fprintf( stderr, "ConnectedClient::Send: OutLock.Unlock: %s\n", SDL_GetError() );
}


void ConnectedClient::SendOthers( Packet *packet )
{
	return Raptor::Server->Net.SendAllExcept( packet, this );
}


void ConnectedClient::SendPing( void )
{
	uint8_t ping_id = 0;
	bool found_available = false;
	
	for( int i = 0; i <= 255; i ++ )
	{
		if( SentPings.find( i ) == SentPings.end() )
		{
			ping_id = i;
			found_available = true;
			break;
		}
	}
	
	if( ! found_available )
	{
		for( std::map<uint8_t,Clock>::iterator ping_iter = SentPings.begin(); ping_iter != SentPings.end(); )
		{
			std::map<uint8_t,Clock>::iterator ping_next = ping_iter;
			ping_next ++;
			
			if( ping_iter->second.ElapsedSeconds() > 3. )
			{
				ping_id = ping_iter->first;
				SentPings.erase( ping_iter );
				found_available = true;
				break;
			}
			
			ping_iter = ping_next;
		}
	}
	
	if( found_available )
	{
		SentPings[ ping_id ].Reset();
		
		Packet ping( Raptor::Packet::PING );
		ping.AddUChar( ping_id );
		Send( &ping );
	}
}


double ConnectedClient::LatestPing( void )
{
	return PingTimes.size() ? PingTimes.back() : 0.;
}


double ConnectedClient::AveragePing( void )
{
	return PingTimes.size() ? Num::Avg(PingTimes) : 0.;
}


double ConnectedClient::MedianPing( void )
{
	return PingTimes.size() ? Num::Med(PingTimes) : 0.;
}


// -----------------------------------------------------------------------------


int ConnectedClient::ConnectedClientInThread( void *client )
{
	ConnectedClient *connected_client = (ConnectedClient *) client;
	char data[ PACKET_BUFFER_SIZE ] = "";
	PacketBuffer Buffer;
	
	while( connected_client->Connected )
	{
		// Check for packets.
		int size = 0;
		if( ( size = SDLNet_TCP_Recv( connected_client->Socket, data, PACKET_BUFFER_SIZE ) ) > 0 )
		{
			// If the main server thread has dropped this client, don't try to process the incoming packet.
			if( ! connected_client->Connected )
				break;
			
			connected_client->BytesReceived += size;
			Buffer.AddData( data, size );
			
			while( Packet *packet = Buffer.Pop() )
			{
				if( ! connected_client->InLock.Lock() )
					fprintf( stderr, "ConnectedClientInThread: connected_client->InLock.Lock: %s\n", SDL_GetError() );
				
				// While locked, add an incoming packet to the input buffer.
				connected_client->InBuffer.push( packet );
				
				if( ! connected_client->InLock.Unlock() )
					fprintf( stderr, "ConnectedClientInThread: connected_client->InLock.Unlock: %s\n", SDL_GetError() );
			}
		}
		else
		{
			// If 0 (disconnect) or -1 (error), stop listening.
			connected_client->Disconnect();
			break;
		}
		
		// Let the thread rest a bit.
		SDL_Delay( 1 );
	}
	
	// Set the thread pointer to NULL so we can delete this client.
	connected_client->InThread = NULL;
	
	return 0;
}


int ConnectedClient::ConnectedClientOutThread( void *client )
{
	ConnectedClient *connected_client = (ConnectedClient *) client;
	
	while( connected_client->Connected )
	{
		if( ! connected_client->OutBuffer.empty() )
		{
			if( ! connected_client->OutLock.Lock() )
				fprintf( stderr, "ConnectedClientOutThread: connected_client->OutLock.Lock: %s\n", SDL_GetError() );
			
			// While locked, snag a copy of the entire output buffer and clear the original.
			std::queue< Packet*, std::list<Packet*> > prev_out_buffer = connected_client->OutBuffer;
			connected_client->OutBuffer = std::queue< Packet*, std::list<Packet*> >();
			
			if( ! connected_client->OutLock.Unlock() )
				fprintf( stderr, "ConnectedClientOutThread: connected_client->OutLock.Unlock: %s\n", SDL_GetError() );
			
			while( ! prev_out_buffer.empty() )
			{
				Packet *packet = prev_out_buffer.front();
				prev_out_buffer.pop();
				connected_client->SendNow( packet );
				delete( packet );
			}
		}
		
		// Let the thread rest a bit.
		SDL_Delay( 1 );
	}
	
	// Set the thread pointer to NULL so we can delete this client.
	connected_client->OutThread = NULL;
	
	return 0;
}
