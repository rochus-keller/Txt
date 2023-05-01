#ifndef __TextHtmlImporter__
#define __TextHtmlImporter__

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Copyright 2013-2017 Rochus Keller <mailto:me@rochus-keller.ch>
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

#include <Txt/TextHtmlParser.h>
#include <QtGui/QTextDocument>
#include <QtGui/QTextTableCell>
#include <QPointer>

namespace Txt
{
	class LinkRendererInterface;

    class TextHtmlImporter : public Txt::TextHtmlParser
    {
        struct Table;
    public:
        enum ImportMode {
            ImportToFragment,
            ImportToDocument
        };

        TextHtmlImporter(QTextDocument *_doc, const QString &html,
                          ImportMode mode = ImportToDocument,
                          const QTextDocument *resourceProvider = 0);

        void import();
		void setLinkRenderer( const LinkRendererInterface* lri ) { d_lri = lri; }
		void setBlockFormat(const QTextBlockFormat &format);
    private:
        bool closeTag();

        Table scanTable(int tableNodeIdx);

        enum ProcessNodeResult { ContinueWithNextNode, ContinueWithCurrentNode, ContinueWithNextSibling };

        void appendBlock(const QTextBlockFormat &format, QTextCharFormat charFmt = QTextCharFormat());
        bool appendNodeText();

        ProcessNodeResult processBlockNode();
        ProcessNodeResult processSpecialNodes();

        struct List
        {
            inline List() : listNode(0) {}
            QTextListFormat format;
            int listNode;
            QPointer<QTextList> list;
        };
        QVector<List> lists;
        int indent;

        // insert a named anchor the next time we emit a char format,
        // either in a block or in regular text
        QStringList namedAnchors;

        struct TableCellIterator
        {
            inline TableCellIterator(QTextTable *t = 0) : table(t), row(0), column(0) {}

            inline TableCellIterator &operator++() {
                if (atEnd())
                    return *this;
                do {
                    const QTextTableCell cell = table->cellAt(row, column);
                    if (!cell.isValid())
                        break;
                    column += cell.columnSpan();
                    if (column >= table->columns()) {
                        column = 0;
                        ++row;
                    }
                } while (row < table->rows() && table->cellAt(row, column).row() != row);

                return *this;
            }

            inline bool atEnd() const { return table == 0 || row >= table->rows(); }

            QTextTableCell cell() const { return table->cellAt(row, column); }

            QTextTable *table;
            int row;
            int column;
        };

        friend struct Table;
        struct Table
        {
            Table() : isTextFrame(false), rows(0), columns(0), currentRow(0), lastIndent(0) {}
            QPointer<QTextFrame> frame;
            bool isTextFrame;
            int rows;
            int columns;
            int currentRow; // ... for buggy html (see html_skipCell testcase)
            TableCellIterator currentCell;
            int lastIndent;
        };
        QVector<Table> tables;

        struct RowColSpanInfo
        {
            int row, col;
            int rowSpan, colSpan;
        };

        enum WhiteSpace
        {
            RemoveWhiteSpace,
            CollapseWhiteSpace,
            PreserveWhiteSpace
        };

        WhiteSpace compressNextWhitespace;

        QTextDocument *doc;
        QTextCursor cursor;
        TextHtmlParserNode::WhiteSpaceMode wsm;
        ImportMode importMode;
        bool hasBlock;
        bool forceBlockMerging;
        bool blockTagClosed;
        int currentNodeIdx;
        const TextHtmlParserNode *currentNode;
		const LinkRendererInterface* d_lri;
    };
}

#endif
