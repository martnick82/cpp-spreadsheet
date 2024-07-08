#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <list>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Wrong position"s);
    }
    CellInterface* temp = sheet_[pos].release();
    try
    {
        sheet_[pos].reset(new Cell(*this, text));
        std::vector<Position> ref_cells = sheet_[pos]->GetReferencedCells();
        if (std::binary_search(ref_cells.begin(), ref_cells.end(), pos))
        {
            sheet_[pos].reset(temp);
            throw CircularDependencyException("Reference to itself"s);
        }
        DeleteVirtualCells(pos);
        for (Position rpos : ref_cells)
        {
            if (!GetCell(rpos))
            {
                SetVCell(rpos, pos);
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (const FormulaException&)
    {
        sheet_[pos].reset(temp);
        throw;
    }
    catch (const InvalidPositionException&)
    {
        sheet_[pos].reset(temp);
        throw;
    }
    catch (const CircularDependencyException&)
    {
        sheet_[pos].reset(temp);
        throw;
    }
    if (virtual_cells_.count(pos))
    {
        virtual_cells_.erase(pos);
    }
    if (!IsInPrintableArea(pos))
    {
        SetNewPrintableArea(pos);
    }
}

void Sheet::SetVCell(Position pos, Position depending_pos)
{
    if (!pos.IsValid() || !depending_pos.IsValid())
    {
        throw InvalidPositionException("Wrong position"s);
    }
    
    virtual_cells_[pos].insert(depending_pos);
   
    if (sheet_.count(pos))
    {
        sheet_.erase(pos);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const 
{
    return const_cast<Sheet&>(*this).GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) 
{
    if (!pos.IsValid())
    { 
        throw InvalidPositionException("Wrong position"s);
    }
    if (virtual_cells_.count(pos))
    {
        return EMPTY_CELL.get();
    }
    if (!IsInPrintableArea(pos) || !sheet_.count(pos))
    {
        return nullptr;
    }
    return sheet_.at(pos).get();
}

void Sheet::ClearCell(Position pos) 
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Wrong position"s);
    }
    if (!sheet_.count(pos))
    {
        return;
    }
    sheet_.erase(pos);
    DeleteVirtualCells(pos);
    if (sheet_.empty() )
    {
        print_size_ = { 0, 0 };
    }
    else if ((pos.row == (print_size_.rows - 1)) || (pos.col == (print_size_.cols - 1)))
    {
        print_size_ = GetPrintSize();
    }
}

Size Sheet::GetPrintableSize() const 
{
    return print_size_;
}

void Sheet::PrintValues(std::ostream& output) const 
{
    Print(output);
}

void Sheet::PrintTexts(std::ostream& output) const
{
    Print(output, true);
}

namespace service_spreadsheet
{
    std::ostream& operator<<(std::ostream &output, const CellInterface::Value& value) {
        std::visit(
            [&output](const auto& x) {
                output << x;
            },
            value);
        return output;
    }
}// service_spreadsheet

void Sheet::Print(std::ostream& output, bool text) const
{
    using namespace service_spreadsheet;
    for (int row = 0; row < print_size_.rows; ++row)
    {
        bool first = true;
        for (int col = 0; col < print_size_.cols; ++col)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                output << '\t';
            }
            Position pos = { row, col };
            if (GetCell(pos))
            {
                output << (text ? GetCell(pos)->GetText() : GetCell(pos)->GetValue());
            }
        }
        output << '\n';
    }
}

bool Sheet::IsInPrintableArea(Position pos) const
{
    return pos.IsValid() && pos.col < print_size_.cols && pos.row < print_size_.rows;
}

Size Sheet::GetPrintSize()
{
    auto row_it = std::max_element(sheet_.begin(), sheet_.end(), 
        [](const auto& lhs, const auto& rhs) {
                return lhs.first.row < rhs.first.row;
        });
    auto col_it = std::max_element(sheet_.begin(), sheet_.end(),
        [](const auto& lhs, const auto& rhs) {
                return lhs.first.col < rhs.first.col;
        });
    return { row_it->first.row + 1, col_it->first.col + 1 };
}

// Íåò ïðîâåðêè âàëèäíîñòè. ÂÛÇÎÂ ÒÎËÜÊÎ ÏÎÑËÅ ÏÐÎÂÅÐÊÈ pos.IsValid()
void Sheet::SetNewPrintableArea(Position pos)
{
    if (pos.col > print_size_.cols - 1)
    {
        print_size_.cols = pos.col + 1;
    }
    if (pos.row > print_size_.rows - 1)
    {
        print_size_.rows = pos.row + 1;
    }
}

std::optional<std::vector<Position>>  Sheet::IsEmptyReference(const Position pos) const
{
    std::list<Position> cells;
    for (auto& [vpos, ref] : virtual_cells_)
    {
        if (ref.count(pos))
        {
            cells.push_back(vpos);
        }
    }
    if (cells.empty())
    {
         return {};
    }
    return { { cells.begin(), cells.end() } };
}

void Sheet::DeleteVirtualCells(const Position pos)
{
    if (auto v_pos = IsEmptyReference(pos))
    {
        std::list<Position> empty;
        for (const Position ref_pos : *v_pos)
        {
            virtual_cells_[ref_pos].erase(pos);
            if (virtual_cells_[ref_pos].empty())
            {
                empty.push_back(ref_pos);
            }
        }
        for (const Position empty_pos : empty)
        {
            virtual_cells_.erase(empty_pos);
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet()
{
    return std::make_unique<Sheet>();
}
