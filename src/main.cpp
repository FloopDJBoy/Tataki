#include <QApplication>
#include <QPushButton>

#include "Uci.h"
#include "ChessCore/FenHelper.h"
#include "Gui/Board.h"
#include "ChessCore/preft.h"
#include "Gui/MainWindow.h"
int main(int argc, char *argv[]) {
    //QApplication app(argc, argv);
    //auto pos = ChessCore::Position(ChessCore::FenHelper::STARTING_POSITION);
    //assert(t == 20);
    UCI::loop();
    return 0;
}
