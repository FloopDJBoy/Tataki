
#include "Board.h"
#include <QPainter>
#include <QSvgRenderer>
#include <optional>
#include <QMouseEvent>
namespace ChessGui {
    Board::Board(ChessCore::Position& position,QWidget *parent)
    : QWidget(parent), pos(position)
    {
        setMinimumSize(800, 800);
    }
    void Board::paintEvent(QPaintEvent *event) {
        Q_UNUSED(event);

        QPainter painter(this);

        const int boardSize = qMin(width(), height()) - margin * 2;
        const int squareSize = boardSize / 8;

        const int offsetX = (width() - boardSize) / 2;
        const int offsetY = (height() - boardSize) / 2;

        // Draw board
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {

                QRectF rect(
                    offsetX + (col) * squareSize,
                    offsetY + (row) * squareSize,
                    squareSize,
                    squareSize
                );

                if ((row + col) % 2 == 0)
                    painter.fillRect(rect, Board::square_light);
                else
                    painter.fillRect(rect, Board::square_dark);
                const Square s =  cord_to_square(col,7-row);
                if (highlighted_squares.contains(s)) {
                    QPointF center = rect.center();
                    qreal radius = squareSize * 0.2;
                    painter.setBrush(Qt::white);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(center, radius, radius);
                }
                const Piece p = pos.square(s);
                if (p != ChessCore::Pieces::EMPTY) {
                    if (drag_state.is_in_drag && drag_state.from == convert_cords(s)) {
                        continue;
                    }
                    m_renderers[p]->render(&painter, rect);
                }
            }
        }
        if (drag_state.is_in_drag) {
            const QRectF r(
                drag_state.mouse_pos.x() - offsetX,
                drag_state.mouse_pos.y() - offsetY,
                squareSize,
                squareSize
                );
            m_renderers[drag_state.piece]->render(&painter, r);
        }
        // Draw labels
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 14));

        QString files = "abcdefgh";

        // Files (a-h)
        for (int col = 0; col < 8; col++) {
            QRectF rect(
                offsetX + col * squareSize,
                offsetY + boardSize,
                squareSize,
                margin
            );

            painter.drawText(rect, Qt::AlignCenter, QString(files[col]));
        }

        // Ranks (1-8)
        for (int row = 0; row < 8; row++) {
            QRectF rect(
                offsetX - margin,
                offsetY + row * squareSize,
                margin,
                squareSize
            );

            painter.drawText(rect, Qt::AlignCenter, QString::number(8 - row));
        }
    }

    void Board::mousePressEvent(QMouseEvent *event) {
        const auto square = square_at(event->pos());
        if (!square) {
            return;
        }
        const auto piece = pos.square(convert_cords(*square));
        auto set = pos.get_moves_squares(convert_cords(*square));
        highlighted_squares.clear();
        highlighted_squares.insert(set.begin(),set.end());
        if (piece == ChessCore::Pieces::EMPTY || ChessCore::Pieces::getColor(piece) != pos.side_to_move()) {
            return;
        }
        drag_state.is_in_drag = true;
        drag_state.from = *square ;
        drag_state.piece = piece;
        drag_state.mouse_pos = event->pos();

    }
    std::optional<Square> Board::square_at(const QPoint& q_pos) const {
        const int boardSize = qMin(width(), height()) - margin * 2;
        const int squareSize = boardSize / 8;
        const int offsetX = (width() - boardSize) / 2;
        const int offsetY = (height() - boardSize) / 2;
        if (q_pos.x() < offsetX ||q_pos.x() >= offsetX + boardSize ||q_pos.y() < offsetY ||q_pos.y() >= offsetY + boardSize)
            return std::nullopt;
        const int col = (q_pos.x() - offsetX) / squareSize;
        const int row = (q_pos.y() - offsetY) / squareSize;
        return cord_to_square(col, row);
    }
    void Board::mouseMoveEvent(QMouseEvent *event) {
        if (drag_state.is_in_drag) {
            drag_state.mouse_pos = event->pos();
            update();
        }
    }
    void Board::mouseReleaseEvent(QMouseEvent *event) {
        if (drag_state.is_in_drag) {
            drag_state.is_in_drag = false;
            highlighted_squares.clear();
            auto s = square_at(event->pos());
            if (!s || *s == drag_state.from) {
                update();
                return;
            }
            const ChessCore::Move m(convert_cords(drag_state.from), convert_cords(*s));
            pos.try_make_move(m);
            update();
        }
    }

}
