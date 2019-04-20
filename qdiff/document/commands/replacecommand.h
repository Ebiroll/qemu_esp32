#ifndef REPLACECOMMAND_H
#define REPLACECOMMAND_H

#include "hexcommand.h"

class ReplaceCommand: public HexCommand
{
    public:
        ReplaceCommand(QHexBuffer* buffer, int offset, const QByteArray& data, QUndoCommand* parent = 0);
        virtual void undo();
        virtual void redo();

    private:
        QByteArray m_olddata;
};

#endif // REPLACECOMMAND_H
