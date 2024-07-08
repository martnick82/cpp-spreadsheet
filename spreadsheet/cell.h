#pragma once

#include "common.h"
#include "formula.h"

#include <optional>

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    Cell(SheetInterface& sheet, std::string text);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;

private:
    class Impl;
    class TextImpl;
    class EmptyImpl;
    class FormulaImpl;
    
    std::unique_ptr<Impl> impl_;
    std::string text_value_;
    SheetInterface& sheet_;
};
