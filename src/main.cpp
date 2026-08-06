#include <QApplication>
#include <QPushButton>

#include "ChessCore/FenHelper.h"
#include "Gui/Board.h"
#include "ChessCore/preft.h"
#include "Gui/MainWindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    auto pos = ChessCore::Position(ChessCore::FenHelper::KIWIPETE);
    ChessCore::preft::test(pos,5);
    //assert(t == 20);
    ChessGui::Board board(pos);
    board.show();
    return QApplication::exec();
}
