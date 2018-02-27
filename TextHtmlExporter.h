#ifndef TextHtmlExporter_H
#define TextHtmlExporter_H

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Copyright 2013-2017 Rochus Keller <mailto:me@rochus-keller.info>
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/QTextDocument>
#include <QtGui/QTextFrame>

class QTextTable;
namespace Txt
{
    class TextHtmlExporter
    {
    public:
        TextHtmlExporter(const QTextDocument *_doc);

        enum ExportMode {
            ExportEntireDocument,
            ExportFragment
        };

        QString toHtml(const QByteArray &encoding, ExportMode mode = ExportEntireDocument);
        void setQtSpecifics(bool on){ qtSpecifics = on; }
    private:
        enum StyleMode { EmitStyleTag, OmitStyleTag };
        enum FrameType { TextFrame, TableFrame, RootFrame };

        void emitFrame(QTextFrame::Iterator frameIt);
        void emitTextFrame(const QTextFrame *frame);
        void emitBlock(const QTextBlock &block);
        void emitTable(const QTextTable *table);
        void emitFragment(const QTextFragment &fragment);

        void emitBlockAttributes(const QTextBlock &block);
        bool emitCharFormatStyle(const QTextCharFormat &format);
        void emitTextLength(const char *attribute, const QTextLength &length);
        void emitAlignment(Qt::Alignment alignment);
        void emitFloatStyle(QTextFrameFormat::Position pos, StyleMode mode = EmitStyleTag);
        void emitMargins(const QString &top, const QString &bottom, const QString &left, const QString &right);
        void emitAttribute(const char *attribute, const QString &value);
        void emitFrameStyle(const QTextFrameFormat &format, FrameType frameType);
        void emitBorderStyle(QTextFrameFormat::BorderStyle style);
        void emitPageBreakPolicy(QTextFormat::PageBreakFlags policy);

        void emitFontFamily(const QString &family);

        void emitBackgroundAttribute(const QTextFormat &format);
        QString findUrlForImage(const QTextDocument *doc, qint64 cacheKey, bool isPixmap);

        QString html;
        QTextCharFormat defaultCharFormat;
        const QTextDocument *doc;
        bool fragmentMarkers;
        bool qtSpecifics;
    };
}

#endif
