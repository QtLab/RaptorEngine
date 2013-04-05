/*
 *  Pos.cpp
 */

#include "Pos.h"

#include <cmath>


Pos3D::Pos3D( void )
{
	SetPos( 0., 0., 0. );
	SetFwdVec( 0., 0., -1. );
	SetUpVec( 0., 1., 0. );
}

Pos3D::Pos3D( const Pos3D &other )
{
	Copy( &other );
}

Pos3D::Pos3D( const Pos3D *other )
{
	Copy( other );
}

Pos3D::Pos3D( double x, double y, double z )
{
	SetPos( x, y, z );
	SetFwdVec( 0., 0., -1. );
	SetUpVec( 0., 1., 0. );
}

Pos3D::~Pos3D()
{
}


void Pos3D::Copy( const Pos3D *other )
{
	SetPos( other->X, other->Y, other->Z );
	SetFwdVec( other->Fwd.X, other->Fwd.Y, other->Fwd.Z );
	SetUpVec( other->Up.X, other->Up.Y, other->Up.Z );
}


void Pos3D::SetPos( double x, double y, double z )
{
	X = x;
	Y = y;
	Z = z;
}

void Pos3D::SetFwdVec( double u, double v, double w )
{
	Fwd.X = u;
	Fwd.Y = v;
	Fwd.Z = w;
	Fwd.ScaleTo( 1. );
	UpdateRight();
}

void Pos3D::SetUpVec( double u, double v, double w )
{
	Up.X = u;
	Up.Y = v;
	Up.Z = w;
	Up.ScaleTo( 1. );
	UpdateRight();
}

void Pos3D::SetFwdPt( double x, double y, double z )
{
	Fwd.X = x - X;
	Fwd.Y = y - Y;
	Fwd.Z = z - Z;
	Fwd.ScaleTo( 1. );
}

void Pos3D::SetUpPt( double x, double y, double z )
{
	Up.X = x - X;
	Up.Y = y - Y;
	Up.Z = z - Z;
	Up.ScaleTo( 1. );
}

void Pos3D::UpdateRight( void )
{
	Right = Fwd.Cross( &Up );
}

void Pos3D::FixVectors( void )
{
	Fwd.ScaleTo( 1. );
	Up.ScaleTo( 1. );
	UpdateRight();
	Right.ScaleTo( 1. );
	Up = Right.Cross( &Fwd );
}


void Pos3D::Yaw( double degrees )
{
	Fwd.RotateAround( &Up, -degrees );
	UpdateRight();
}

void Pos3D::Pitch( double degrees )
{
	Fwd.RotateAround( &Right, degrees );
	Up.RotateAround( &Right, degrees );
}

void Pos3D::Roll( double degrees )
{
	Up.RotateAround( &Fwd, degrees );
	UpdateRight();
}


void Pos3D::Move( double dx, double dy, double dz )
{
	X += dx;
	Y += dy;
	Z += dz;
}


void Pos3D::MoveAlong( const Vec3D *vec, double dist )
{
	if( vec )
		MoveAlong( vec->X, vec->Y, vec->Z, dist );
}


void Pos3D::MoveAlong( double w, double u, double v, double dist )
{
	Vec3D vec( w, u, v );
	vec.ScaleTo( 1. );
	Move( vec.X * dist, vec.Y * dist, vec.Z * dist );
}


double Pos3D::AheadX( void ) const
{
	return X + Fwd.X;
}

double Pos3D::AheadY( void ) const
{
	return Y + Fwd.Y;
}

double Pos3D::AheadZ( void ) const
{
	return Z + Fwd.Z;
}

double Pos3D::AboveX( void ) const
{
	return X + Up.X;
}

double Pos3D::AboveY( void ) const
{
	return Y + Up.Y;
}

double Pos3D::AboveZ( void ) const
{
	return Z + Up.Z;
}


double Pos3D::Dist( const Pos3D *other ) const
{
	return sqrt( ((X - other->X) * (X - other->X)) + ((Y - other->Y) * (Y - other->Y)) + ((Z - other->Z) * (Z - other->Z)) );
}


double Pos3D::DistAlong( const Vec3D *vec, const Pos3D *from ) const
{
	if( !( vec && from ) )
		return 0.;
	
	Vec3D diff( X - from->X, Y - from->Y, Z - from->Z );
	return diff.Dot( vec );
}


Pos3D *Pos3D::Nearest( const std::vector<Pos3D*> *others )
{
	if( ! others )
		return NULL;
	
	Pos3D *nearest = NULL;
	double nearest_dist = 0.;
	
	for( std::vector<Pos3D*>::const_iterator pos_iter = others->begin(); pos_iter != others->end(); pos_iter ++ )
	{
		double dist = Dist(*pos_iter);
		if( (dist < nearest_dist) || (! nearest) )
		{
			nearest = *pos_iter;
			nearest_dist = dist;
		}
	}
	
	return nearest;
}


Pos3D *Pos3D::Nearest( const std::list<Pos3D*> *others )
{
	if( ! others )
		return NULL;
	
	Pos3D *nearest = NULL;
	double nearest_dist = 0.;
	
	for( std::list<Pos3D*>::const_iterator pos_iter = others->begin(); pos_iter != others->end(); pos_iter ++ )
	{
		double dist = Dist(*pos_iter);
		if( (dist < nearest_dist) || (! nearest) )
		{
			nearest = *pos_iter;
			nearest_dist = dist;
		}
	}
	
	return nearest;
}


Pos3D &Pos3D::operator +=( const Vec3D &vec )
{
	X += vec.X;
	Y += vec.Y;
	Z += vec.Z;
	return *this;
}


Pos3D &Pos3D::operator -=( const Vec3D &vec )
{
	X -= vec.X;
	Y -= vec.Y;
	Z -= vec.Z;
	return *this;
}


Pos3D &Pos3D::operator +=( const Pos3D &other )
{
	X += other.X;
	Y += other.Y;
	Z += other.Z;
	return *this;
}


Pos3D &Pos3D::operator -=( const Pos3D &other )
{
	X -= other.X;
	Y -= other.Y;
	Z -= other.Z;
	return *this;
}