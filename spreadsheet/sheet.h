#pragma once

#include "cell.h"
#include "common.h"

#include <unordered_set>
#include <functional>

using CellStorage = std::unordered_map<Position, std::unique_ptr<CellInterface>, PositionHasher>;
using VirtualCellIndex = std::unordered_map<Position, std::unordered_set<Position, PositionHasher>, PositionHasher>;

class Sheet : public SheetInterface {
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    //создание виртуальной €чейки, значение которой не задано, но есть ссылка на неЄ
    void SetVCell(Position pos, Position depending_pos);

    const CellInterface* GetCell(Position pos) const override;
          CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;

    void PrintTexts(std::ostream& output) const override;

private:
    CellStorage sheet_;
    VirtualCellIndex virtual_cells_;
    Size print_size_;

    const  std::unique_ptr<CellInterface> EMPTY_CELL = std::unique_ptr<Cell>(new Cell(*this));

    bool IsInPrintableArea(Position pos) const;

    Size GetPrintSize();

    void Print(std::ostream& output, bool text = false) const;

    void SetNewPrintableArea(const Position pos);

    std::optional<std::vector<Position>> IsEmptyReference(const Position pos) const;

    void DeleteVirtualCells(const Position pos);
};


