
#pragma once
#include <QSvgRenderer>
#include <qwidget.h>
#include <string>
#include "Types.h"
#include "ChessCore/Pieces.h"
#include "magic_enum.hpp"
#include "ChessCore/Board.h"
#include "ChessCore/Position.h"
using namespace std::string_literals;
namespace ChessGui {
    class Board : public QWidget  {
        Q_OBJECT
        constexpr static int margin = 30;
        ChessCore::Position& pos;
        [[nodiscard]] std::optional<Square> square_at(const QPoint& q_pos) const ;
        std::set<Square> highlighted_squares = std::set<Square>();
        struct DragState {
            bool is_in_drag = false;
            Square from{};
            Piece piece{};
            QPoint mouse_pos;
        };
        DragState drag_state;
        public:
        inline const static QHash<Piece,QSharedPointer<QSvgRenderer>> m_renderers = []()-> QHash<Piece,QSharedPointer<QSvgRenderer>> {
            auto hash = QHash<Piece,QSharedPointer<QSvgRenderer>>();
            for (const auto c : magic_enum::enum_values<Color>()) {
                for (const auto p : magic_enum::enum_values<PieceType>()) {
                    if (p == PieceType::EMPTY) {continue;}
                    hash.emplace(
                        ChessCore::Pieces::makePiece(p,c),
                         QSharedPointer<QSvgRenderer>::create(
                            QString(":/assets/pieces/") +
                            QString::fromStdString(std::string(magic_enum::enum_name(p))).toLower() +
                            "_" +
                            QString::fromStdString(std::string(magic_enum::enum_name(c))).toLower() +
                            ".svg"
                        )
                    );                }
            }
            return hash;
        }();
        const QColor square_light = QColor::fromString("#E3C16F");
        const QColor square_dark   = QColor::fromString("#B88B4A");
        [[nodiscard]] static Square convert_cords(const Square s) {return (7 - (s / 8)) * 8 + (s % 8); }
        [[nodiscard]]
        explicit Board(ChessCore::Position& position,QWidget *parent = nullptr);
        protected:
        void paintEvent(QPaintEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
    };
}



