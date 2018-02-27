/*
* Copyright 2009-2017 Rochus Keller <mailto:me@rochus-keller.info>
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

#include "LinkIcon.h"
#include "Styles.h"
#include <QFontMetricsF>
#include <QTextDocument>
#include <QPainter>
#include <QPicture>
#include <Stream/DataCell.h> // Picture Metatype
using namespace Txt;

LinkIcon::LinkIcon(QObject* p):
	QObject( p )
{

}

void LinkIcon::regHandler(const QTextDocument* doc )
{	
	if( dynamic_cast<LinkIcon*>(doc->documentLayout()->handlerForObject( Type )) == 0 )
		doc->documentLayout()->registerHandler( Type, 
			new LinkIcon( const_cast<QTextDocument*>(doc) ) );
}

QSizeF LinkIcon::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    QTextCharFormat tf = format.toCharFormat();
    QFontMetricsF fm( tf.font() );

    QSizeF size( fm.height(), fm.height() );
    size.setHeight( size.height() - fm.descent() - 1.0 );
    // Wenn man die Höhe verändert, wird das Bild gestaucht.
	return size;
}

void LinkIcon::drawObject(QPainter *p, const QRectF &r, QTextDocument *doc, 
				int posInDocument, const QTextFormat &format)
{
	QTextCharFormat tf = format.toCharFormat();
    QFontMetricsF fm( tf.font() );
    // Qt positioniert das Bild auf die Grundlinie der Schrift.
	// Da in intrinsicSize die Höhe gestaucht, mache hier nach unten wieder wett.
    QRectF rr = r.adjusted( 0, 0, 0, fm.descent() + 1.0 );

    QPixmap pix( tf.stringProperty( Styles::PropIcon ) );
    QRectF pr = pix.rect();
    if( pr.width() < rr.width() && pr.height() < rr.height() )
    {
        pr.moveCenter( rr.center() );
        rr = pr;
    }
    p->drawPixmap( rr.toRect(), pix );
}
