#pragma once
#include <Vireio.h>
#include <qstring.h>
#include <d3dx9.h>
#include <functional>

#include "InputControls.h"


class D3DProxyDevice;
class cMenu;

class cHotkey{
public:
	InputControls* input;
	QVector<int>   codes;

	cHotkey();

	bool    active( const QVector<int>& down );
	QString toString();
	QString toCodeString();
	void    fromCodeString( const QString& );
	bool    valid();
	void    clear();
	bool    listen();
};




class cMenuItem{
public:
	bool showCalibrator;
	bool visible;
	bool readOnly;

	bool  internalBool;
	int   internalInt;

	std::function<void()> callback;

	cMenuItem* addSubmenu ( const QString& name );
	cMenuItem* addAction  ( const QString& name );
	cMenuItem* addSpinner ( const QString& name , float*   variable , float min , float max , float step );
	cMenuItem* addSpinner ( const QString& name , float*   variable , float step );
	cMenuItem* addSpinner ( const QString& name , int*     variable , int min , int max , int step );
	cMenuItem* addCheckbox( const QString& name , bool*    variable , const QString& on_text="true" , const QString& off_text="false" );
	cMenuItem* addSelect  ( const QString& name , int*     variable , const QStringList& variants );
	cMenuItem* addText    ( const QString& name , QString* variable );
	void removeChildren();

	~cMenuItem();

private:
	enum TYPE{
		SUBMENU,
		ACTION,
		SPINNER,
		CHECKBOX,
		SELECT,
		TEXT,
	};
	
	QString               name;
	QList<cMenuItem*>     children;
	cMenuItem*            parent;
	cMenuItem*            selected;
	TYPE                  type;
	bool                  spinLimit;
	float                 spinMin;
	float                 spinMax;
	float                 spinStep;
	float*                spinVarFlt;
	int*                  spinVarInt;
	bool*                 checkVar;
	int*                  selectVar;
	QStringList           selectVariants;
	QString*              textVar;
	QString               checkOn;
	QString               checkOff;
	cHotkey               hotkey;


	cMenuItem();

	cMenuItem* add        ( const QString& name , TYPE type );
	void       trigger    ( float k=0 );
	QString    path       ( );
	friend class cMenu;
};



class cMenu {
public:
	int             viewportWidth;
	int             viewportHeight;
	cMenuItem       root;
	bool            show;

	void init           ( D3DProxyDevice* d );
	void createResources( );
	void freeResources  ( );
	void render         ( );
	void saveHotkeys    ( cMenuItem* item=0 );
	void showMessage    ( const QString& text );
	void goToMenu       ( cMenuItem* item=0 );

private:
	enum{
		ALIGN_CENTER,
		ALIGN_HOTKEY_COLUMN,
		ALIGN_LEFT_COLUMN,
		ALIGN_RIGHT_COLUMN
	};

	D3DProxyDevice* device;
	ID3DXSprite*    sprite;
	ID3DXFont*      font;

	int             posY;
	int             scrollY;
	cMenuItem*      menu;
	int             keyDelay;
	bool            prevKeyDown;
	bool            newKeyDown;
	int             hotkeyState;
	cHotkey         hotkeyNew;
	QTime           hotkeyTimeout;
	QString         messageText;
	QTime           messageTimeout;
	QTime           keyTime;

	void drawText( const QString& text , int align );
	void drawRect( int x1 , int y1 , int x2 , int y2 , int color );
	void checkHotkeys( cMenuItem* i , const QVector<int>& down );
	void updateSelected( );
};




