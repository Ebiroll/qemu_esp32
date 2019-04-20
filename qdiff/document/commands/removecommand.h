#ifndef REMOVECOMMAND_H
#define REMOVECOMMAND_H

#include "hexcommand.h"

class RemoveCommand: public HexCommand
{
    public:
        RemoveCommand(QHexBuffer* buffer, int offset, int length, QUndoCommand* parent = 0);
        virtual void undo();
        virtual void redo();
};

#endif // REMOVECOMMAND_H
