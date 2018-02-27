/*
* Copyright 2007-2017 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the CrossLine Txt library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.info.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/
#include "TextScrollCtrl.h"
#include "TextView.h"
#include "TextCtrl.h"
#include <QScrollArea>
using namespace Txt;

TextScrollCtrl::TextScrollCtrl( QWidget* p, TextView* v ):QWidget(p)
{
	TextCtrl* tc = new TextCtrl( this, v );
	d_sa = new QScrollArea( this );
	d_sa->setWidgetResizable( true );
	d_sa->setWidget( tc );
	connect( tc, SIGNAL( cursorPositionChanged() ),
			this, SLOT( handleCursorChanged() ) );
}

void TextScrollCtrl::ensureVisible(const QRect &rect)
{
    if (rect.y() < scrollPos() )
        setScrollPos( rect.y() - rect.height());
    else if (rect.y() + rect.height() > scrollPos() + visibleHeight() )
        setScrollPos( rect.y() + rect.height() - visibleHeight() );
}

void TextScrollCtrl::handleCursorChanged()
{
// TODO	ensureVisible( view()->cursorRect().toRect() );
}

void TextScrollCtrl::setScrollPos( int pos )
{
	/* TODO
	if( d_vscroll )
		d_vscroll->setValue( pos );
		*/
}

int TextScrollCtrl::scrollPos()
{
	return 0; // TODO
}

int TextScrollCtrl::visibleHeight()
{
	return 0; // TODO
}
