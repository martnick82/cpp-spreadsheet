#include "cell.h"

#include <unordered_set>
#include <list>


class Cell::Impl
{
public:
    virtual Value GetValue() const = 0;
    virtual std::vector<Position> GetReferencedCells() const;
    virtual ~Impl() = default;
    void SetDependence(const Impl* cell);

    virtual const std::unordered_set<const Impl*> GetDependenceCell() const;
    virtual void InvalidateCache() const;
    virtual void ClearCache() const {}

private:
    std::unordered_set<const Impl*> depending_cells_;
};

//-----Implementation Impl------
void Cell::Impl::SetDependence(const Impl* cell)
{
    depending_cells_.emplace(cell);
}

const std::unordered_set<const Cell::Impl*> Cell::Impl::GetDependenceCell() const
{
    return depending_cells_;
}
void Cell::Impl::InvalidateCache() const
{
    for (const Impl* cell : depending_cells_)
    {
        cell->ClearCache();
    }
}
std::vector<Position> Cell::Impl::GetReferencedCells() const
{
    return std::vector<Position>();
}

class Cell::EmptyImpl : public Cell::Impl
{
public:
    Value GetValue() const override
    {
        using namespace std::string_literals;
        return Value(0.0);
    }

    std::vector<Position> GetReferencedCells() const override
    {
        return {};
    }
};

class Cell::TextImpl : public Cell::Impl
{
public:
    explicit TextImpl(std::string_view text)
        : value_(text) {}

    Value GetValue() const override
    {
        try
        {
            return std::stod(std::string(value_.data(), value_.size()));
        }
        catch(std::exception&)
        {
            return std::string(value_.data(), value_.size());;
        }
    }
private:
    std::string_view value_;
};

class Cell::FormulaImpl : public Cell::Impl
{
public:
    explicit FormulaImpl(SheetInterface& sheet, std::string_view text);

    Value GetValue() const override;

    std::vector<Position> GetReferencedCells() const override;

    std::string GetExpression() const;

    void ClearCache() const override;

    ~FormulaImpl() = default;

private:
    SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cache_value_;
    std::unordered_set<const FormulaImpl*> depending_cells_;

    void InvalidateCache() const;

    //ѕровер€ет, ссылаетс€ ли €чейка на другие €чейки
    bool IsMoreCell(std::list<Position>& cell_list, CellInterface* cell);

    //ѕроверка наличи€ циклов в графе из €чеек
    bool IsCycleRef(std::vector<Position> cells);
};

//-----Implementation FormulaImpl------

Cell::FormulaImpl::FormulaImpl(SheetInterface& sheet, std::string_view text)
    : sheet_(sheet)
{
    FormulaInterface* f_temp = formula_.release();
    try
    {
        formula_ = ParseFormula(std::string(text.data(), text.size()));
    }
    catch (const FormulaException& error)
    {
        formula_.reset(f_temp);
        throw;
    }
    catch (const std::exception& error)
    {
        formula_.reset(f_temp);
        throw FormulaException(error.what());
    }
    std::vector<Position> ref_cells = formula_->GetReferencedCells();
    if (!ref_cells.empty() && IsCycleRef(ref_cells))
    {
        throw CircularDependencyException("Circular dependency detecting"s);
    }
}

std::string Cell::FormulaImpl::GetExpression() const
{
    return formula_->GetExpression();
}

void Cell::FormulaImpl::ClearCache() const
{
    cache_value_.reset();
    InvalidateCache();
}

CellInterface::Value Cell::FormulaImpl::GetValue() const
{
    if (!cache_value_)
    {
        cache_value_ = formula_->Evaluate(sheet_);
        InvalidateCache();
    }
    if (std::holds_alternative<double>(*cache_value_))
    {
        return std::get<double>(*cache_value_);
    }
    else
    {
        return std::get<FormulaError>(*cache_value_);
    }
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const
{
    return formula_->GetReferencedCells();
}
void  Cell::FormulaImpl::InvalidateCache() const
{
    for (const Impl* cell : depending_cells_)
    {
        cell->ClearCache();
    }
}

bool Cell::FormulaImpl::IsMoreCell(std::list<Position>& cell_list, CellInterface* cell)
{
    if (!cell)
    {
        return false;
    }
    std::vector<Position>&& vect = cell->GetReferencedCells();
    if (vect.empty())
    {
        return false;
    }
    cell_list.splice(cell_list.begin(), { vect.begin(), vect.end() });
    return true;
}

bool Cell::FormulaImpl::IsCycleRef(std::vector<Position> cells)
{
    using CellSet = std::unordered_set<Position, PositionHasher>;;
    CellSet visited; //black colored
    CellSet gray_list; //gray colored
    std::list<Position> white_list; //white colored
    white_list.splice(white_list.begin(), { cells.begin(), cells.end() });
    if (white_list.empty())
    {
        return false;
    }
    gray_list.emplace(white_list.front());
    CellInterface* cell_ptr = sheet_.GetCell(white_list.front());
    white_list.pop_front();
    if (!IsMoreCell(white_list, cell_ptr))
    {
        return false;
    }
    Position cell = white_list.front();
    while (!white_list.empty())
    {
        if (visited.count(cell))
        {
            cell = white_list.front();
            white_list.pop_front();
            continue;
        }
        if (gray_list.count(cell))
        {
            return true;
        }
        cell_ptr = sheet_.GetCell(cell);
        if (!IsMoreCell(white_list, cell_ptr))
        {
            visited.insert(cell);
        }
        else
        {
            gray_list.insert(cell);
        }
        cell = white_list.front();
        white_list.pop_front();
    }
    if (gray_list.count(cell))
    {
        return true;
    }
    return false;
}

Cell::Cell(SheetInterface& sheet, std::string text)
	: Cell(sheet) 
{
    Set(text);
}

Cell::Cell(SheetInterface& sheet)
    : impl_(std::make_unique<EmptyImpl>(EmptyImpl())), sheet_(sheet) {}

Cell::~Cell() = default;

#define CATCH_RESTORE_IMPL(exc) catch(exc){   restore();  throw;  }
void Cell::Set(std::string text)
{	
    std::string temp_string = text_value_;
	text_value_ = std::move(text);
    std::unique_ptr<Impl> temp_ptr(impl_.release());
    const auto restore = [&]()
        {
            impl_.reset(temp_ptr.release());
            text_value_ = temp_string;
        };
	if (text_value_.empty())
	{
        try
        {
            impl_.reset(new EmptyImpl());
        }
        CATCH_RESTORE_IMPL(std::bad_alloc&)
		return;
	}
	else if (text_value_.size() > 1 && text_value_.at(0) == FORMULA_SIGN)
	{
        try 
		{
			impl_.reset(new FormulaImpl(sheet_, std::string_view(text_value_.data() + 1, text_value_.size() - 1)));
		}
        CATCH_RESTORE_IMPL(const FormulaException&)
        CATCH_RESTORE_IMPL(const CircularDependencyException&)
        text_value_ = FORMULA_SIGN + dynamic_cast<FormulaImpl*>(impl_.get())-> GetExpression();
	}
	else
    {
        std::string_view text = text_value_;
        if (text_value_.at(0) == ESCAPE_SIGN)
        {
            text.remove_prefix(1);
        }
        try
        {
            impl_.reset(new TextImpl(text));
        }
        CATCH_RESTORE_IMPL(std::bad_alloc&)
	}
}

void Cell::Clear()
{
    std::unordered_set<const Impl*> temp = impl_->GetDependenceCell();
    if (temp.empty())
    {
        impl_.release();
    }
    else
    {
        impl_->InvalidateCache();
        try
        {
            impl_.reset(new EmptyImpl());
        }
        catch (std::bad_alloc&)
        {
            impl_.release();
            throw;
        }
    }
}

Cell::Value Cell::GetValue() const 
{
	return impl_->GetValue();
}

std::string Cell::GetText() const 
{
	return text_value_;
}

std::vector<Position> Cell::GetReferencedCells() const
{

    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const
{
    return !impl_->GetReferencedCells().empty();
}
