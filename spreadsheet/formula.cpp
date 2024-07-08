#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) try
            : ast_(ParseFormulaAST(expression)){}
        catch (const FormulaException&)
        {
            throw;
        }
        catch (const std::exception& error)
        {
            throw FormulaException(error.what());
        }
        Value Evaluate(const SheetInterface& sheet) const override;
        std::string GetExpression() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        FormulaAST ast_;
    };
    FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const
    {
        try
        {
            return (ast_.Execute(sheet));
        }
        catch (const FormulaError& error)
        {
            return { error };
        }
    }
    std::string Formula::GetExpression() const
    {
        std::ostringstream str_out;
        ast_.PrintFormula(str_out);
        return str_out.str();
    }
    std::vector<Position> Formula::GetReferencedCells() const
    {
        std::forward_list<Position> result = ast_.GetCells();
        result.unique();
        return { result.begin(), result.end() };
    }
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}