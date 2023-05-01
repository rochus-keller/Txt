/*
* Copyright 2007-2017 Rochus Keller <mailto:me@rochus-keller.ch>
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

#include "TextOutHtml.h"
#include "LinkRendererInterface.h"
#include <QTextList>
#include <QTextTable>
#include <QVarLengthArray>
#include <QFileInfo>
#include <Stream/DataCell.h>
#include <Txt/ImageGlyph.h>
#include <Txt/Styles.h>
using namespace Txt;

// TextOutHtml::transformAnchor wird nur noch in FlowLine1 verwendet! Wird nicht mehr unterstützt.

QString TextOutHtml::htmlToPlainText( const QString& html, bool simlified )
{
	const QString res = Stream::DataCell::stripMarkup( html, true );
    if( simlified )
        return res.simplified();
    else
        return res;
}

QString LinkRendererInterface::renderHref( const QByteArray& link ) const
{
	// Default-Implementation
	return defaultRenderHref(link);
}

QString LinkRendererInterface::defaultRenderHref(const QByteArray &link)
{
	return QString("%1%2").arg( QLatin1String( Styles::s_linkSchema ) ).
            arg( QString::fromUtf8( link.toBase64() ) );
}

bool LinkRendererInterface::renderLink( TextCursor&, const QByteArray& ) const
{
	return false;
}

QString TextOutHtml::linkToHtml(const QByteArray & data, const QString &text)
{
    QString ending;
    if( text.isEmpty() )
        ending = QLatin1String( "/>" );
    else
		ending = QString( ">%1</a>" ).arg( text ); // TODO: escape?
	return QString("<a href=\"%1\"%3").arg( LinkRendererInterface::defaultRenderHref(data) ).arg( ending );
}

TextOutHtml::TextOutHtml(const QTextDocument *_doc, bool emitP )
	: doc(_doc), d_firstPar( true ), d_emitP( emitP ),d_lri(0),d_noFileDirs(false)
{
}

/*!
    Returns the document in HTML format. The conversion may not be
    perfect, especially for complex documents, due to the limitations
    of HTML.
*/
QString TextOutHtml::toHtml(bool fragmentOnly, const QByteArray &charset)
{
	if( !fragmentOnly )
    {
        if( !charset.isEmpty() )
            html = QString( "<html><META http-equiv=\"Content-Type\""
                   "content=\"text/html; charset=%1\"><body>" ).arg( QString::fromLatin1(charset) );
        else
            html = QLatin1String("<html><body>");
    }
    //html.reserve(doc->docHandle()->length());

	d_firstPar = true;
    emitFrame(doc->rootFrame()->begin());

	if( !fragmentOnly )
		html += QLatin1String("</body></html>");
    return html;
}

void TextOutHtml::emitAttribute(const char *attribute, const QString &value)
{
    html += QLatin1Char(' ');
    html += QLatin1String(attribute);
    html += QLatin1String("=\"");
    html += value;
    html += QLatin1Char('"');
}

void TextOutHtml::emitFragments(const QTextBlock &block)
{
	// NOTE: dieser Code kann auch mit Links umgehen, die sich mit gleichem PropAnchorId über mehrere
	// Fragmente erstrecken.
	QTextBlock::Iterator it = block.begin();
	while( !it.atEnd() )
	{
		const QTextCharFormat format = it.fragment().charFormat();

		bool closeAnchor = false;
		bool multiFragmentLink = false;
		int linkId = 0;
		if( format.isAnchor() )
		{
			if( format.hasProperty( Styles::PropLink ) )
			{
				if( format.hasProperty(Styles::PropAnchorId) )
				{
					multiFragmentLink = true;
					linkId = format.intProperty( Styles::PropAnchorId );
				}
				if( d_lri )
					html += QString("<a href=\"%1\">").arg(
								d_lri->renderHref( format.property( Styles::PropLink ).toByteArray() ) );
				else
					html += QString("<a href=\"%1\">").arg( LinkRendererInterface::defaultRenderHref(
																format.property( Styles::PropLink ).toByteArray() ) );
			}else
			{
				if( d_lri && format.anchorHref().startsWith( QLatin1String( Styles::s_linkSchema ) ) )
				{
					html += QString("<a href=\"%1\">").arg(
								d_lri->renderHref( QByteArray::fromBase64(
                                format.anchorHref().mid( ::strlen( Styles::s_linkSchema ) ).toUtf8() ) ) );
				}
				else
				{
                    QUrl url = QUrl::fromEncoded( format.anchorHref().toUtf8() );
					if( d_noFileDirs && url.scheme().startsWith( QLatin1String("file"), Qt::CaseInsensitive ) )
						url = QUrl::fromLocalFile( QFileInfo( url.toLocalFile() ).fileName() );
					html += QString("<a href=\"%1\">").arg( url.toEncoded().data() );
				}
			}
			closeAnchor = true;
		}

		do
		{
			QString txt = it.fragment().text();
			if( it.fragment().length() == 1 && txt.at(0) == QChar::ObjectReplacementCharacter )
				emitImage( it.fragment().charFormat().toImageFormat() );
			else
			{
				// Q_ASSERT(!txt.contains(QChar::ObjectReplacementCharacter));
				// Die Assertion trifft wegen Links mit Icons nicht mehr zu

				txt.replace( QChar::ObjectReplacementCharacter, QChar(' ' ) );
                emitText( it.fragment().charFormat(), txt.toHtmlEscaped() );
			}
			if( multiFragmentLink )
			{
				it++;
				if( it.atEnd() || it.fragment().charFormat().intProperty( Styles::PropAnchorId ) != linkId )
					multiFragmentLink = false;
			}else
				it++;
		}while( multiFragmentLink );

		if (closeAnchor)
			html += QLatin1String("</a>");
	}
}

static bool isOrderedList(int style)
{
    return style == QTextListFormat::ListDecimal || style == QTextListFormat::ListLowerAlpha
           || style == QTextListFormat::ListUpperAlpha;
}

void TextOutHtml::emitBlock(const QTextBlock &block)
{
	/*
    if (block.begin().atEnd()) 
	{
        // ### HACK, remove once QTextFrame::Iterator is fixed
        int p = block.position();
        if (p > 0)
            --p;
        QTextDocumentPrivate::FragmentIterator frag = doc->docHandle()->find(p);
        QChar ch = doc->docHandle()->buffer().at(frag->stringPosition);
        if (ch == QTextBeginningOfFrame || ch == QTextEndOfFrame)
            return;
    }
	*/

	if( block.text().isEmpty() )
		return;

    html += QLatin1Char('\n');

    QTextList *list = block.textList();
    if (list) 
	{
        if (list->itemNumber(block) == 0) 
		{ // first item? emit <ul> or appropriate
            const QTextListFormat format = list->format();
            const int style = format.style();
            switch (style) 
			{
                case QTextListFormat::ListDecimal: html += QLatin1String("<ol>"); break;
                case QTextListFormat::ListDisc: html += QLatin1String("<ul>"); break;
                case QTextListFormat::ListCircle: html += QLatin1String("<ul type=\"circle\">"); break;
                case QTextListFormat::ListSquare: html += QLatin1String("<ul type=\"square\">"); break;
                case QTextListFormat::ListLowerAlpha: html += QLatin1String("<ol type=\"a\">"); break;
                case QTextListFormat::ListUpperAlpha: html += QLatin1String("<ol type=\"A\">"); break;
                default: html += QLatin1String("<ul"); // ### should not happen
            }
        }

        html += QLatin1String("<li>");

    }

    const bool pre = block.blockFormat().nonBreakableLines();
    if (pre) 
	{
        html += QLatin1String("<pre>");
    } else if (!list) 
	{
		if( d_emitP )
			html += QLatin1String("<p>");
		else if( !d_firstPar )
			html += QLatin1String("<br>");
    }
	emitFragments( block );

    if (pre)
        html += QLatin1String("</pre>");
    else if (list)
        html += QLatin1String("</li>");
    else if( d_emitP )
        html += QLatin1String("</p>");

    if (list) 
	{
        if (list->itemNumber(block) == list->count() - 1) 
		{ // last item? close list
            if (isOrderedList(list->format().style()))
                html += QLatin1String("</ol>");
            else
                html += QLatin1String("</ul>");
        }
    }
	d_firstPar = false;
}

void TextOutHtml::emitTable(const QTextTable *table)
{
    QTextTableFormat format = table->format();

    html += QLatin1String("\n<table border=1 cellspacing=0 CELLPADDING=3 >"); // RISK

    const int rows = table->rows();
    const int columns = table->columns();

    const int headerRowCount = qMin(format.headerRowCount(), rows);
    if (headerRowCount > 0)
        html += QLatin1String("<thead>");

    for (int row = 0; row < rows; ++row) 
	{
        html += QLatin1String("\n<tr>");

        for (int col = 0; col < columns; ++col) 
		{
            const QTextTableCell cell = table->cellAt(row, col);

            // for col/rowspans
            if (cell.row() != row)
                continue;

            if (cell.column() != col)
                continue;

            html += QLatin1String("\n<td");

            if (cell.columnSpan() > 1)
                emitAttribute("colspan", QString::number(cell.columnSpan()));

            if (cell.rowSpan() > 1)
                emitAttribute("rowspan", QString::number(cell.rowSpan()));

            html += QLatin1Char('>');

            emitFrame(cell.begin());

            html += QLatin1String("</td>");
        }

        html += QLatin1String("</tr>");
        if (headerRowCount > 0 && row == headerRowCount - 1)
            html += QLatin1String("</thead>");
    }

    html += QLatin1String("</table>");
}

void TextOutHtml::emitFrame(QTextFrame::Iterator frameIt)
{
    if (!frameIt.atEnd()) 
	{
        QTextFrame::Iterator next = frameIt;
        ++next;
        if (next.atEnd()
            && frameIt.currentFrame() == 0
            && frameIt.parentFrame() != doc->rootFrame()
            && frameIt.currentBlock().begin().atEnd())
            return;
    }

    for (QTextFrame::Iterator it = frameIt; !it.atEnd(); ++it) 
	{
        if (QTextFrame *f = it.currentFrame()) 
		{
            if (QTextTable *table = qobject_cast<QTextTable *>(f)) 
			{
                emitTable(table);
            } else {
                emitFrame(f->begin());
            }
        } else if (it.currentBlock().isValid()) 
		{
            emitBlock(it.currentBlock());
        }
	}
}

void TextOutHtml::emitImage(const QTextImageFormat & imgFmt)
{
	html += QLatin1String("<img");

	if( imgFmt.objectType() == ImageGlyph::Type )
	{
		if( imgFmt.hasProperty( Styles::PropSize ) )
		{
			const QSize s = imgFmt.property( Styles::PropSize ).value<QSize>();
			emitAttribute("width", QString::number(s.width()));
			emitAttribute("height", QString::number(s.height()));
		}
		Stream::DataCell v;
		QVariant var = imgFmt.property( Styles::PropImage );
		if( var.canConvert<QImage>() )
			v.setImage( var.value<QImage>() );
		else if( var.canConvert<QPixmap>() )
			v.setImage( var.value<QPixmap>().toImage() );
		if( !v.isNull() )
		{
			html += " alt=\"<embedded image>\" src=\"data:image/png;base64,";
			html += v.getArr().toBase64();
			html += "\"";
		}
	}else
	{
		if (imgFmt.hasProperty(QTextFormat::ImageName))
		{
			const QString name = imgFmt.name();
			emitAttribute("src", name);
		}

		if (imgFmt.hasProperty(QTextFormat::ImageWidth))
			emitAttribute("width", QString::number(imgFmt.width()));

		if (imgFmt.hasProperty(QTextFormat::ImageHeight))
			emitAttribute("height", QString::number(imgFmt.height()));
	}

	html += QLatin1String(" />");
}

void TextOutHtml::emitText(const QTextCharFormat & format, const QString & txt)
{
	// split for [\n{LineSeparator}]
	QString forcedLineBreakRegExp = QString::fromLatin1("[\\na]");
	forcedLineBreakRegExp[3] = QChar::LineSeparator;

	QFont f = format.font();
	if( f.italic() )
		html += "<i>";
	if( f.bold() )
		html += "<b>";
	if( f.underline() )
		html += "<u>";
	if( f.strikeOut() )
		html += "<del>";
	if( format.verticalAlignment() == QTextCharFormat::AlignSuperScript )
		html += "<sup>";
	if( format.verticalAlignment() == QTextCharFormat::AlignSubScript )
		html += "<sub>";
	if( format.fontFixedPitch() )
		html += "<code>";
	const QStringList lines = txt.split(QRegExp(forcedLineBreakRegExp));
	for( int i = 0; i < lines.count(); ++i )
	{
		if (i > 0)
			html += QLatin1String("<br />"); // space on purpose for compatibility with Netscape, Lynx & Co.
		html += lines.at(i);
	}
	if( format.fontFixedPitch() )
		html += "</code>";
	if( format.verticalAlignment() == QTextCharFormat::AlignSubScript )
		html += "</sub>";
	if( format.verticalAlignment() == QTextCharFormat::AlignSuperScript )
		html += "</sup>";
	if( f.strikeOut() )
		html += "</del>";
	if( f.underline() )
		html += "</u>";
	if( f.bold() )
		html += "</b>";
	if( f.italic() )
		html += "</i>";
}
