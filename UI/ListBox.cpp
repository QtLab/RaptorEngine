/*
 *  ListBox.cpp
 */

#include "ListBox.h"
#include "RaptorGame.h"


ListBox::ListBox( Layer *container, SDL_Rect *rect, Font *font, int scroll_bar_size ) : Layer( container, rect )
{
	Selected = NULL;
	TextFont = font;
	
	Red = 0.0f;
	Green = 0.0f;
	Blue = 0.0f;
	Alpha = 0.75f;
	
	TextRed = 1.0f;
	TextGreen = 1.0f;
	TextBlue = 1.0f;
	TextAlpha = 1.0f;
	
	SelectedRed = 1.0f;
	SelectedGreen = 1.0f;
	SelectedBlue = 0.0f;
	SelectedAlpha = 1.0f;
	
	ScrollBarSize = scroll_bar_size;
	ScrollBarRed = 1.0f;
	ScrollBarGreen = 1.0f;
	ScrollBarBlue = 1.0f;
	ScrollBarAlpha = 1.0f;
	
	Scroll = 0;
	
	AddElement( new ListBoxButton( this, ListBox::SCROLL_UP, Raptor::Game->Res.GetAnimation("arrow_up.ani"), Raptor::Game->Res.GetAnimation("arrow_up_mdown.ani") ) );
	AddElement( new ListBoxButton( this, ListBox::SCROLL_DOWN, Raptor::Game->Res.GetAnimation("arrow_down.ani"), Raptor::Game->Res.GetAnimation("arrow_down_mdown.ani") ) );
}


ListBox::ListBox( Layer *container, SDL_Rect *rect, Font *font, int scroll_bar_size, std::vector<ListBoxItem> items ) : Layer( container, rect )
{
	Selected = NULL;
	TextFont = font;
	
	Red = 0.0f;
	Green = 0.0f;
	Blue = 0.0f;
	Alpha = 0.75f;
	
	TextRed = 1.0f;
	TextGreen = 1.0f;
	TextBlue = 1.0f;
	TextAlpha = 1.0f;
	
	SelectedRed = 1.0f;
	SelectedGreen = 1.0f;
	SelectedBlue = 0.0f;
	SelectedAlpha = 1.0f;
	
	ScrollBarSize = scroll_bar_size;
	ScrollBarRed = 1.0f;
	ScrollBarGreen = 1.0f;
	ScrollBarBlue = 1.0f;
	ScrollBarAlpha = 1.0f;
	
	Scroll = 0;
	
	AddElement( new ListBoxButton( this, ListBox::SCROLL_UP, Raptor::Game->Res.GetAnimation("arrow_up.ani"), Raptor::Game->Res.GetAnimation("arrow_up_mdown.ani") ) );
	AddElement( new ListBoxButton( this, ListBox::SCROLL_DOWN, Raptor::Game->Res.GetAnimation("arrow_down.ani"), Raptor::Game->Res.GetAnimation("arrow_down_mdown.ani") ) );
	
	Items = items;
}


ListBox::~ListBox()
{
}


void ListBox::AddItem( std::string value, std::string text )
{
	Items.push_back( ListBoxItem( value, text ) );
}


int ListBox::FindItem( std::string value )
{
	int size = Items.size();
	for( int i = 0; i < size; i ++ )
	{
		try
		{
			if( Items.at( i ).Value == value )
				return i;
		}
		catch( std::out_of_range &exception )
		{
			fprintf( stderr, "ListBox::FindItem: std::out_of_range\n" );
		}
	}

	return -1;
}


void ListBox::RemoveItem( std::string value )
{
	int index = FindItem( value );
	RemoveItem( index );
}


void ListBox::RemoveItem( int index )
{
	if( index >= 0 )
	{
		try
		{
			// Create an iterator so the vector::erase method will work properly.
			std::vector<ListBoxItem>::iterator iter = Items.begin() + index;
			Items.erase( iter );
		}
		catch( std::out_of_range &exception )
		{
			fprintf( stderr, "ListBox::RemoveItem: std::out_of_range\n" );
		}
	}
}


void ListBox::Clear( void )
{
	Items.clear();
	Selected = NULL;
}


int ListBox::LineScroll( void )
{
	if( TextFont )
		return TextFont->GetLineSkip();
	return 10;
}


int ListBox::MaxScroll( void )
{
	int max_scroll = 0;
	max_scroll = Items.size() * LineScroll() - Rect.h;
	
	if( max_scroll >= 0 )
		return max_scroll;
	return 0;
}


void ListBox::ScrollUp( void )
{
	Scroll -= LineScroll();
	
	if( Scroll < 0 )
		Scroll = 0;
}


void ListBox::ScrollDown( void )
{
	Scroll += LineScroll();
	
	int max_scroll = MaxScroll();
	if( Scroll > max_scroll )
		Scroll = max_scroll;
}


void ListBox::UpdateRects( void )
{
	// FIXME: This method assumes the only elements are ListBoxButtons.
	for( std::vector<Layer*>::iterator element_iter = Elements.begin(); element_iter != Elements.end(); element_iter ++ )
	{
		ListBoxButton *button = (ListBoxButton*)(*element_iter);
		button->UpdateRect();
	}
}


void ListBox::Draw( void )
{
	UpdateRects();
	
	glColor4f( Red, Green, Blue, Alpha );
	glBegin( GL_QUADS );
		glVertex2i( 0, 0 );
		glVertex2i( Rect.w, 0 );
		glVertex2i( Rect.w, Rect.h );
		glVertex2i( 0, Rect.h );
	glEnd();
	
	/*
	int max_scroll = MaxScroll();
	if( max_scroll > 0 )
	{
		// FIXME: Draw scroll bar!
		glColor4f( ScrollBarRed, ScrollBarGreen, ScrollBarBlue, ScrollBarAlpha );
		glBegin( GL_LINES )
			glVertex2i( x, y );
			glVertex2i( x+w, y );
		glEnd();
	}
	*/
	
	glPushMatrix();
	Raptor::Game->Gfx.Setup2D( 0, 0, Rect.w - ScrollBarSize, Rect.h );
	glViewport( CalcRect.x, Raptor::Game->Gfx.H - CalcRect.y - Rect.h, Rect.w - ScrollBarSize, Rect.h );
	
	if( TextFont )
	{
		int y = -Scroll;
		for( std::vector<ListBoxItem>::iterator iter = Items.begin(); iter != Items.end(); iter ++ )
		{
			if( (y < Rect.h) && ((y + LineScroll()) >= 0) )
			{
				if( Selected == &(*iter) )
					TextFont->DrawText( (*iter).Text, 0, y, Font::ALIGN_TOP_LEFT, SelectedRed, SelectedGreen, SelectedBlue, SelectedAlpha );
				else
					TextFont->DrawText( (*iter).Text, 0, y, Font::ALIGN_TOP_LEFT, TextRed, TextGreen, TextBlue, TextAlpha );
			}
			y += LineScroll();
		}
	}
	
	glPopMatrix();
	glViewport( 0, 0, Raptor::Game->Gfx.W, Raptor::Game->Gfx.H );
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}


void ListBox::MouseEnter( void )
{
}


void ListBox::MouseLeave( void )
{
}


bool ListBox::MouseDown( Uint8 button )
{
	return true;
}


bool ListBox::MouseUp( Uint8 button )
{
	if( button == SDL_BUTTON_WHEELDOWN )
		ScrollDown();
	else if( button == SDL_BUTTON_WHEELUP )
		ScrollUp();
	else
	{
		int y = Raptor::Game->Mouse.Y - CalcRect.y;
		
		int index = (y + Scroll) / LineScroll();
		
		if( (size_t) index < Items.size() )
		{
			try
			{
				Selected = &(Items.at( index ));
				Changed();
			}
			catch( std::out_of_range &exception )
			{
				fprintf( stderr, "ListBox::MouseUp: std::out_of_range\n" );
			}
		}
	}
	
	return true;
}


void ListBox::Changed( void )
{
}


std::string ListBox::SelectedValue( void )
{
	if( Selected )
		return Selected->Value;
	return "";
}


void ListBox::Select( std::string value )
{
	for( std::vector<ListBoxItem>::iterator iter = Items.begin(); iter != Items.end(); iter ++ )
	{
		if( (*iter).Value == value )
		{
			Selected = &(*iter);
			Changed();
			break;
		}
	}
}


void ListBox::Select( int index )
{
	if( index < 0 )
	{
		Selected = NULL;
		Changed();
	}
	else if( (size_t) index < Items.size() )
		Select( Items[ index ].Value );
}


// ---------------------------------------------------------------------------


ListBoxItem::ListBoxItem( std::string value, std::string text )
{
	Value = value;
	Text = text;
}


// ---------------------------------------------------------------------------


ListBoxButton::ListBoxButton( ListBox *list_box, uint8_t action, Animation *normal, Animation *mouse_down ) : Button( list_box, NULL, normal, mouse_down )
{
	Container = list_box;
	Action = action;
	
	UpdateRect();
}


void ListBoxButton::UpdateRect( void )
{
	ListBox *list_box = (ListBox*) Container;
	
	int scroll_button_h = list_box->ScrollBarSize;
	if( scroll_button_h * 2 > list_box->Rect.h )
		scroll_button_h = list_box->Rect.h / 2;
	
	if( Action == ListBox::SCROLL_UP )
	{
		Rect.x = list_box->Rect.w - list_box->ScrollBarSize;
		Rect.y = 0;
		Rect.w = list_box->ScrollBarSize;
		Rect.h = scroll_button_h;
	}
	else if( Action == ListBox::SCROLL_DOWN )
	{
		Rect.x = list_box->Rect.w - list_box->ScrollBarSize;
		Rect.y = list_box->Rect.h - list_box->ScrollBarSize;
		Rect.w = list_box->ScrollBarSize;
		Rect.h = scroll_button_h;
	}
}


void ListBoxButton::Clicked( Uint8 button )
{
	ListBox *list_box = (ListBox*) Container;
	if( (Action == 1) && list_box )
		list_box->ScrollUp();
	else if( (Action == 2) && list_box )
		list_box->ScrollDown();
}
