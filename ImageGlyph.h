#ifndef _TEXT_IMAGEGLYPH
#define _TEXT_IMAGEGLYPH

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

#include <QObject>
#include <QAbstractTextDocumentLayout>
#include <QTextCharFormat>
#include <QImage>

namespace Txt
{
	class ImageGlyph : public QObject, public QTextObjectInterface  
	{
		Q_OBJECT
		Q_INTERFACES(QTextObjectInterface)
	public:
		enum { Type = QTextFormat::UserObject + 1 };

		ImageGlyph( QObject* );
		static void regHandler( const QTextDocument*);
		static QImage parseDataSrc( const QString& );

		// Overrides
		QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format);
		void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format);

	};
}

#endif // _TEXT_IMAGEGLYPH
