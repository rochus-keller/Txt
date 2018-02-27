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

#include "TextEdit.h"
#include <QTextDocument>
#include <QPainter>
#include <Gui2/AutoMenu.h>
#include <QApplication>
#include <QClipboard>
#include <QtDebug>
#include <Stream/DataWriter.h>
#include "TextHtmlExporter.h"
#include "TextOutStream.h"
using namespace Txt;

TextEdit::TextEdit(QWidget *parent, Txt::Styles *s) :
    QWidget(parent),d_hasFocus(false)
{
    setFocusPolicy( Qt::StrongFocus );
    setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Fixed );

    Txt::TextView* tv = new Txt::TextView( this, s, true, false );
    d_txt = new Txt::TextCtrl( this, tv );

    QRect cr = contentsRect();
    d_txt->setPos( cr.left(), cr.top() );
    tv->setWidth( cr.width(), cr.height() );

    connect( d_txt,SIGNAL(extentChanged()), this,SLOT(onExtentChanged()) );
    connect( d_txt, SIGNAL(invalidate(QRect)), this, SLOT(onInvalidate(QRect)) );
}

void TextEdit::setDefaultFont(const QFont & f)
{
    setFont( f );
    d_txt->getView()->getDocument()->setDefaultFont( f );
}

void TextEdit::setPlainText(const QString & str)
{
    d_txt->getView()->getDocument()->setDefaultFont( font() );
    //d_txt->getView()->getCursor().insertText( str );
    d_txt->getView()->getDocument()->setPlainText( str );
    d_txt->getView()->getDocument()->setModified(false);
}

QString TextEdit::getPlainText() const
{
    return d_txt->getView()->getDocument()->toPlainText();
}

void TextEdit::setHtml(const QString & str)
{
    QTextDocument* doc = new QTextDocument( this );
    doc->setDefaultFont( font() );
    doc->setHtml( str );
    d_txt->getView()->setDocument( doc );
    d_txt->getView()->getDocument()->setModified(false);
    // BUG: fügt leerzeilen an
}

QString TextEdit::getHtml(bool qtSpecifics) const
{
    TextHtmlExporter hexp( d_txt->getView()->getDocument() );
    hexp.setQtSpecifics(qtSpecifics);
    return hexp.toHtml( "utf-8" ).toUtf8();
}

QByteArray TextEdit::getBml() const
{
    Stream::DataWriter w;
    Txt::TextOutStream::writeTo( w, d_txt->getView()->getDocument() );
    return w.getStream();
}

void TextEdit::setBml(const QByteArray & bml)
{
    Txt::TextInStream in;
    Stream::DataReader r( bml );
    d_txt->getView()->setDocument( in.readFrom( r, this ) );
    d_txt->getView()->getDocument()->setModified(false);
}

void TextEdit::clear()
{
    d_txt->getView()->clear();
}

bool TextEdit::isEmpty()
{
    return d_txt->getView()->getDocument()->isEmpty();
}

bool TextEdit::isModified()
{
    return d_txt->getView()->getDocument()->isModified();
}

void TextEdit::setModified(bool y)
{
    d_txt->getView()->getDocument()->setModified( y );
}

void TextEdit::focusInEvent(QFocusEvent *event)
{
    d_txt->handleFocusIn();
    d_hasFocus = true;
    update();
}

void TextEdit::focusOutEvent(QFocusEvent *event)
{
    d_txt->handleFocusOut();
    d_hasFocus = false;
    update();
}

void TextEdit::keyPressEvent(QKeyEvent *event)
{
    d_txt->keyPressEvent( event );
}

void TextEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    d_txt->mouseDoubleClickEvent( event );
}

void TextEdit::mouseMoveEvent(QMouseEvent *event)
{
    d_txt->mouseMoveEvent( event );
}

void TextEdit::mousePressEvent(QMouseEvent *event)
{
    d_txt->mousePressEvent( event );
}

void TextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    d_txt->mouseReleaseEvent( event );
}

void TextEdit::resizeEvent(QResizeEvent *event)
{
    QRect cr = contentsRect();
    d_txt->handleResize( cr.size() );
}

void TextEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter( this );
    d_txt->paint( &painter, palette() );
    painter.setPen( (d_hasFocus)?Qt::darkBlue:Qt::lightGray );
    painter.drawRoundedRect( rect().adjusted( 0, 0, -1, -1 ), 2, 2 );
}

QSize TextEdit::sizeHint() const
{
    return d_txt->getView()->getExtent().toSize();
}

void TextEdit::onExtentChanged()
{
    updateGeometry();
}

void TextEdit::onInvalidate(const QRect & r)
{
    update( r );
}

void TextEdit::onUndo()
{
    ENABLED_IF( d_txt->getView()->getDocument()->isUndoAvailable() );
    d_txt->getView()->undo();
}

void TextEdit::onRedo()
{
    ENABLED_IF( d_txt->getView()->getDocument()->isRedoAvailable() );
    d_txt->getView()->redo();
}

void TextEdit::onCut()
{
    ENABLED_IF( d_txt->getView()->getCursor().hasSelection() );

    d_txt->getView()->copy();
    d_txt->getView()->getCursor().removeSelectedText();
}

void TextEdit::onPaste()
{
    ENABLED_IF( QApplication::clipboard()->mimeData()->hasText() );
	d_txt->getView()->getCursor().insertText( QApplication::clipboard()->mimeData()->text().simplified() );
}

void TextEdit::onPastePlainText()
{
	ENABLED_IF( QApplication::clipboard()->mimeData()->hasText() ||
				QApplication::clipboard()->mimeData()->hasHtml() ||
				QApplication::clipboard()->mimeData()->hasUrls() );
	d_txt->getView()->getCursor().insertText( QApplication::clipboard()->mimeData()->text().simplified() );
}

void TextEdit::onCopy()
{
    ENABLED_IF( d_txt->getView()->getCursor().hasSelection() );
    d_txt->getView()->copy();
}
