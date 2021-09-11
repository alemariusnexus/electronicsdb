/*
    Copyright 2010-2021 David "Alemarius Nexus" Lerch

    This file is part of electronicsdb.

    electronicsdb is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    electronicsdb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with electronicsdb.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PropertyTextEdit.h"

#include <nxcommon/exception/InvalidValueException.h>
#include "../../System.h"

namespace electronicsdb
{


PropertyTextEdit::PropertyTextEdit(PartProperty* prop, QWidget* parent)
        : QPlainTextEdit(parent), cat(prop->getPartCategory()), prop(prop), state(Enabled), flags(0),
          programmaticallyChanging(false)
{
    setFrameShadow(Plain);

    connect(this, &PropertyTextEdit::textChanged, this, &PropertyTextEdit::textChangedSlot);

    setTextValid(true);
}


void PropertyTextEdit::keyPressEvent(QKeyEvent* evt)
{
    EditStack* editStack = System::getInstance()->getEditStack();

    if (evt->matches(QKeySequence::Undo)) {
        if (editStack->canUndo()) {
            editStack->undo();
            evt->accept();
        } else {
            evt->ignore();
        }

        return;
    } else if (evt->matches(QKeySequence::Redo)) {
        if (editStack->canRedo()) {
            editStack->redo();
            evt->accept();
        } else {
            evt->ignore();
        }

        return;
    }

    QPlainTextEdit::keyPressEvent(evt);
}


void PropertyTextEdit::setText(const QString& text)
{
    programmaticallyChanging = true;
    setPlainText(text);
    programmaticallyChanging = false;
}


void PropertyTextEdit::setState(DisplayWidgetState state)
{
    this->state = state;

    applyState();
}


void PropertyTextEdit::setFlags(flags_t flags)
{
    this->flags = flags;

    applyState();
}


QVariant PropertyTextEdit::getValue() const
{
    return prop->parseUserInputValue(toPlainText());
}


void PropertyTextEdit::applyState()
{
    if (state == Enabled) {
        if ((flags & ChoosePart)  !=  0) {
            setEnabled(true);
            setReadOnly(true);
        } else {
            setEnabled(true);
            setReadOnly(false);
        }
    } else if (state == Disabled) {
        setEnabled(false);
    } else if (state == ReadOnly) {
        setEnabled(true);
        setReadOnly(true);
    }
}


void PropertyTextEdit::setTextValid(bool valid)
{
    setStyleSheet(
            QString("\
            QPlainTextEdit { \
                border: 1px solid %1; \
                border-radius: 2px; \
            } \
            QPlainTextEdit:focus { \
                border: 1px solid %2; \
                border-radius: 2px; \
            } \
            ").arg(valid ? "grey" : "red",
                   valid ? "blue" : "red"));
}


void PropertyTextEdit::textChangedSlot()
{
    QString text = toPlainText();

    int maxLen;

    if (	(prop->getFlags() & PartProperty::MultiValue)  !=  0
            &&  (prop->getFlags() & PartProperty::DisplayMultiInSingleField)  !=  0
    ) {
        maxLen = -1;
    } else {
        maxLen = prop->getStringMaximumLength();
    }

    if (maxLen >= 0) {
        if (text.length() > maxLen) {
            unsigned int numDel = text.length() - maxLen;
            int pos = textCursor().position();
            text = text.left(pos-numDel) + text.right(text.length()-pos);
            blockSignals(true);
            setPlainText(text);
            blockSignals(false);
            moveCursor(QTextCursor::End);
        }
    }

    try {
        prop->parseUserInputValue(text);
        setTextValid(true);
    } catch (InvalidValueException&) {
        setTextValid(false);
    }

    if (!programmaticallyChanging)
        emit editedByUser(text);
}


QSize PropertyTextEdit::sizeHint() const
{
    QSize size = QPlainTextEdit::sizeHint();
    size.setHeight(prop->getTextAreaMinimumHeight());
    return size;
}


}
