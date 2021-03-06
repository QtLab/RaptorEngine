/*
 *  DropDown.cpp
 */

#include "DropDown.h"

#include "RaptorGame.h"


DropDown::DropDown( SDL_Rect *rect, Font *font, uint8_t align, int scroll_bar_size, Animation *normal, Animation *mouse_down, Animation *mouse_over )
: LabelledButton( rect, font, "", align, normal, mouse_down, mouse_over )
{
	ScrollBarSize = scroll_bar_size;
	MyListBox = NULL;
}


DropDown::~DropDown()
{
}


void DropDown::AddItem( std::string value, std::string text )
{
	Items.push_back( ListBoxItem( value, text ) );
}


void DropDown::Update( void )
{
	for( std::vector<ListBoxItem>::iterator item_iter = Items.begin(); item_iter != Items.end(); item_iter ++ )
	{
		if( item_iter->Value == Value )
		{
			LabelText = item_iter->Text;
			break;
		}
	}
}


bool DropDown::HandleEvent( SDL_Event *event )
{
	bool handled = Layer::HandleEvent( event );
	
	if( (! handled) && MyListBox && ! MouseIsWithin )
	{
		if( event->type == SDL_MOUSEBUTTONDOWN )
		{
			return true;
		}
		else if( event->type == SDL_MOUSEBUTTONUP )
		{
			Close();
			return true;
		}
	}

	return false;
}


void DropDown::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_WHEELUP )
	{
		// Scrolling up should move to the previous item without showing the listbox.
		
		if( Items.size() )
		{
			bool found = false;
			
			for( size_t i = 1; i < Items.size(); i ++ )
			{
				if( Items[ i ].Value == Value )
				{
					Value = Items[ i - 1 ].Value;
					LabelText = Items[ i - 1 ].Text;
					found = true;
					break;
				}
			}
			
			if( ! found )
			{
				Value = Items.front().Value;
				LabelText = Items.front().Text;
			}
			
			Changed();
		}
	}
	else if( button == SDL_BUTTON_WHEELDOWN )
	{
		// Scrolling down should move to the next item without showing the listbox.
		
		if( Items.size() )
		{
			bool found = false;
			
			for( size_t i = 0; i < Items.size() - 1; i ++ )
			{
				if( Items[ i ].Value == Value )
				{
					Value = Items[ i + 1 ].Value;
					LabelText = Items[ i + 1 ].Text;
					found = true;
					break;
				}
			}
			
			if( ! found )
			{
				Value = Items.back().Value;
				LabelText = Items.back().Text;
			}
			
			Changed();
		}
	}
	else if( ! MyListBox )
	{
		MyListBox = new DropDownListBox(this);
		MyListBox->TextAlign = LabelAlign;
		Container->AddElement( MyListBox );
	}
}


void DropDown::Close( void )
{
	MyListBox->Remove();
	MyListBox = NULL;
}


void DropDown::Changed( void )
{
}


// ---------------------------------------------------------------------------


DropDownListBox::DropDownListBox( DropDown *dropdown )
: ListBox( &(dropdown->Rect), dropdown->LabelFont, dropdown->ScrollBarSize, dropdown->Items )
{
	CalledBy = dropdown;
	
	Alpha = 1.f;
	
	int min_y = CalledBy->Container ? -(CalledBy->Container->CalcRect.y) : 0;
	int max_h = Raptor::Game->Gfx.H;
	
	int selected_index = FindItem( CalledBy->Value );
	if( selected_index >= 0 )
		Rect.y -= selected_index * LineScroll();
	
	Rect.h = Items.size() * LineScroll();
	
	// FIXME: This dirty hack is needed because DropDown::Clicked happens when the dimensions are not the VR eye.
	if( Raptor::Game->Head.VR && Raptor::Game->Cfg.SettingAsBool("vr_enable") )
		return;
	
	if( Rect.h > max_h )
		Rect.h = max_h;
	
	int offscreen = (CalledBy->Container ? CalledBy->Container->CalcRect.y : 0) + Rect.y + Rect.h - Raptor::Game->Gfx.H;
	if( offscreen > 0 )
		Rect.y -= offscreen;
	if( Rect.y < min_y )
		Rect.y = min_y;
}


DropDownListBox::~DropDownListBox()
{
}


void DropDownListBox::Changed( void )
{
	CalledBy->LabelText = SelectedText();
	CalledBy->Value = SelectedValue();
	CalledBy->Changed();
	CalledBy->MyListBox = NULL;
	Remove();
}


bool DropDownListBox::KeyDown( SDLKey key )
{
	if( key == SDLK_ESCAPE )
		return true;
	
	return false;
}


bool DropDownListBox::KeyUp( SDLKey key )
{
	if( key == SDLK_ESCAPE )
	{
		CalledBy->Close();
		return true;
	}
	
	return false;
}
