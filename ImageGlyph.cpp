/*
* Copyright 2009-2017 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the CrossLine Txt library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
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

#include "ImageGlyph.h"
#include "Styles.h"
#include <QFontMetricsF>
#include <QTextDocument>
#include <QPainter>
#include <QPicture>
#include <Stream/DataCell.h> // Picture Metatype
using namespace Txt;

ImageGlyph::ImageGlyph(QObject* p):
	QObject( p )
{

}

void ImageGlyph::regHandler(const QTextDocument* doc )
{	
	if( dynamic_cast<ImageGlyph*>(doc->documentLayout()->handlerForObject( Type )) == 0 )
		doc->documentLayout()->registerHandler( Type, 
												new ImageGlyph( const_cast<QTextDocument*>(doc) ) );
}

QImage ImageGlyph::parseDataSrc(const QString & src)
{
	// src der Form "data:image/png;base64, ..."
	if( !src.startsWith("data:") )
		return QImage();
	const int semi = src.indexOf( QChar(';') );
	if( semi == -1 )
		return QImage();
	QString format = src.mid( 5, semi - 5 );
	if( format.startsWith( QLatin1String("image/") ) )
		format = format.mid(6);
	if( src.mid( semi + 1, 7 ) != QLatin1String( "base64,") )
		return QImage();
	QImage img;
    img.loadFromData( QByteArray::fromBase64(src.mid( semi + 1 + 7 ).toUtf8()), format.toUtf8() );
	return img;
}

QSizeF ImageGlyph::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    QTextCharFormat tf = format.toCharFormat();

	QVariant img = tf.property( Styles::PropImage );
	QSize imgSize;
	if( img.type() == QVariant::Image )
	{
		QImage i = img.value<QImage>();
		imgSize = i.size();
	}else if( img.type() == QVariant::Pixmap )
	{
		QPixmap pix = img.value<QPixmap>();
		imgSize = pix.size();
	}else
		return imgSize;

	return imgSize;
}

void ImageGlyph::drawObject(QPainter *p, const QRectF &r, QTextDocument *doc, 
				int posInDocument, const QTextFormat &format)
{
	QTextCharFormat tf = format.toCharFormat();

	QVariant img = tf.property( Styles::PropImage );
	if( img.canConvert<QImage>() )
	{
		QImage i = img.value<QImage>();
        p->drawImage( r.topLeft(), i );
	}else if( img.canConvert<QPixmap>() )
	{
		QPixmap pix = img.value<QPixmap>();
        p->drawPixmap( r.topLeft(), pix );
	}else if( img.canConvert<QPicture>() )
	{
		QPicture pic = img.value<QPicture>();
        p->drawPicture( r.topLeft(), pic );
	}
}
