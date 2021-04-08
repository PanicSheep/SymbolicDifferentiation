#include "SymExp.h"
#include <cassert>
#include <numeric>

using namespace AST;

int Symbol::counter = 1;

SymExp::SymExp(float value) noexcept : root(std::make_unique<Value>(value))
{}

SymExp::SymExp(std::string name) noexcept : root(std::make_unique<Symbol>(std::move(name)))
{}

SymExp::SymExp(const SymExp& o) noexcept : root(o.root->Clone())
{}

SymExp& SymExp::operator=(const SymExp& o) noexcept
{
	root = o.root->Clone();
	return *this;
}

SymExp operator-(SymExp o)
{
	return SymExp{ std::make_unique<Neg>(std::move(o.root)) };
}

SymExp operator+(SymExp l, SymExp r)
{
	return SymExp{ std::make_unique<Add>(std::move(l.root), std::move(r.root)) };
}

SymExp operator-(SymExp l, SymExp r)
{
	return SymExp{ std::make_unique<Sub>(std::move(l.root), std::move(r.root)) };
}

SymExp operator*(SymExp l, SymExp r)
{
	return SymExp{ std::make_unique<Mul>(std::move(l.root), std::move(r.root)) };
}

SymExp operator/(SymExp l, SymExp r)
{
	return SymExp{ std::make_unique<Div>(std::move(l.root), std::move(r.root)) };
}

SymExp pow(SymExp l, SymExp r)
{
	return SymExp{ std::make_unique<Pow>(std::move(l.root), std::move(r.root)) };
}

SymExp exp(SymExp s)
{
	return SymExp{ std::make_unique<Exp>(std::move(s.root)) };
}

SymExp log(SymExp s)
{
	return SymExp{ std::make_unique<Log>(std::move(s.root)) };
}

SymExp SymExp::At(const Var& var, float value) const
{
	return SymExp{ root->Eval(static_cast<Symbol&>(*var.root), value)->Simplify() };
}

SymExp SymExp::At(const std::vector<Var>& vars, const std::vector<float>& values) const
{
	assert(vars.size() == values.size());
	SymExp tmp = *this;
	for (std::size_t i = 0; i <vars.size(); i++)
		tmp = tmp.At(vars[i], values[i]);
	return tmp;
}

SymExp SymExp::Derive(const Var& var) const
{
	return SymExp{ root->Derive(static_cast<Symbol&>(*var.root))->Simplify() };
}

SymExpVec SymExp::Derive(const std::vector<Var>& vars) const
{
	std::vector<SymExp> ret;
	for (const auto& var : vars)
		ret.push_back(Derive(var));
	return ret;
}

SymExpVec SymExp::DeriveAt(const std::vector<Var>& vars, const std::vector<float>& values) const
{
	assert(vars.size() == values.size());
	std::vector<SymExp> ret;
	for (std::size_t i = 0; i < vars.size(); i++)
		ret.push_back(Derive(vars[i]).At(vars, values));
	return ret;
}

SymExp SymExp::Simplify() const
{
	return SymExp{ root->Simplify() };
}

std::string SymExp::to_string() const
{
	return root->to_string();
}

bool SymExp::has_value() const
{
	return root->has_value();
}

float SymExp::value() const
{
	return root->value();
}


SymExpVec SymExpVec::At(const Var& var, float value) const
{
	std::vector<SymExp> ret;
	ret.reserve(vec.size());
	for (const auto& v : vec)
		ret.push_back(v.At(var, value));
	return { ret };
}

SymExpVec SymExpVec::At(const std::vector<Var>& vars, const std::vector<float>& values) const
{
	std::vector<SymExp> ret;
	ret.reserve(vec.size());
	for (const auto& v : vec)
		ret.push_back(v.At(vars, values));
	return { ret };
}

std::vector<float> SymExpVec::value() const
{
	std::vector<float> ret;
	ret.reserve(vec.size());
	for (const auto& v : vec)
		ret.push_back(v.value());
	return ret;
}


Var::Var() noexcept : SymExp(std::make_unique<Symbol>())
{}

Var::Var(std::string name) noexcept : SymExp(std::make_unique<Symbol>(std::move(name)))
{}

Var::Var(float value) noexcept : SymExp(std::make_unique<Value>(value))
{}



std::unique_ptr<Node> Neg::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Neg>(node->Eval(s, value));
}

std::unique_ptr<Node> Add::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Add>(l->Eval(s, value), r->Eval(s, value));
}

std::unique_ptr<Node> Sub::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Sub>(l->Eval(s, value), r->Eval(s, value));
}

std::unique_ptr<Node> Mul::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Mul>(l->Eval(s, value), r->Eval(s, value));
}

std::unique_ptr<Node> Div::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Div>(l->Eval(s, value), r->Eval(s, value));
}

std::unique_ptr<Node> Pow::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Pow>(l->Eval(s, value), r->Eval(s, value));
}

std::unique_ptr<Node> Exp::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Exp>(node->Eval(s, value));
}

std::unique_ptr<Node> Log::Eval(const Symbol& s, float value) const
{
	return std::make_unique<Log>(node->Eval(s, value));
}



std::unique_ptr<Node> Neg::Derive(const Symbol& s) const
{
	return std::make_unique<Neg>(node->Derive(s));
}

std::unique_ptr<Node> Add::Derive(const Symbol& s) const
{
	return std::make_unique<Add>(l->Derive(s), r->Derive(s));
}

std::unique_ptr<Node> Sub::Derive(const Symbol& s) const
{
	return std::make_unique<Sub>(l->Derive(s), r->Derive(s));
}

std::unique_ptr<Node> Mul::Derive(const Symbol& s) const
{
	return std::make_unique<Add>(
		std::make_unique<Mul>(l->Derive(s), r->Clone()), 
		std::make_unique<Mul>(l->Clone(), r->Derive(s)));
}

std::unique_ptr<Node> Div::Derive(const Symbol& s) const
{
	return std::make_unique<Div>(
		std::make_unique<Sub>(
			std::make_unique<Mul>(l->Derive(s), r->Clone()), 
			std::make_unique<Mul>(l->Clone(), r->Derive(s))),
		std::make_unique<Mul>(r->Clone(), r->Clone()));
}

std::unique_ptr<Node> Pow::Derive(const Symbol& s) const
{
	return std::make_unique<Mul>(
		Clone(), 
		std::make_unique<Add>(
			std::make_unique<Mul>(
				l->Derive(s),
				std::make_unique<Log>(r->Clone())),
			std::make_unique<Mul>(
				l->Clone(),
				std::make_unique<Div>(
					r->Derive(s),
					r->Clone()))
			));
}

std::unique_ptr<Node> Exp::Derive(const Symbol& s) const
{
	return std::make_unique<Mul>(Clone(), node->Derive(s));
}

std::unique_ptr<Node> Log::Derive(const Symbol& s) const
{
	return std::make_unique<Div>(node->Derive(s), Clone());
}



std::unique_ptr<Node> Neg::Simplify() const
{
	auto N = node->Simplify();

	if (N->has_value())
		return std::make_unique<Value>(-N->value());
	if (auto tmp = dynamic_cast<Neg*>(N.get()))
		return tmp->node->Simplify();
	return std::make_unique<Neg>(std::move(N));
}

std::unique_ptr<Node> Add::Simplify() const
{
	auto L = l->Simplify();
	auto R = r->Simplify();

	if (L->has_value() and R->has_value())
		return std::make_unique<Value>(L->value() + R->value());
	if (L->has_value() and L->value() == 0)
		return R;
	if (R->has_value() and R->value() == 0)
		return L;
	return std::make_unique<Add>(std::move(L), std::move(R));
}

std::unique_ptr<Node> Sub::Simplify() const
{
	auto L = l->Simplify();
	auto R = r->Simplify();

	if (L->has_value() and R->has_value())
		return std::make_unique<Value>(L->value() - R->value());
	if (L->has_value() and L->value() == 0)
		return std::make_unique<Neg>(std::move(R));
	if (R->has_value() and R->value() == 0)
		return L;
	return std::make_unique<Sub>(std::move(L), std::move(R));
}

std::unique_ptr<Node> Mul::Simplify() const
{
	auto L = l->Simplify();
	auto R = r->Simplify();

	if (L->has_value() and R->has_value())
		return std::make_unique<Value>(L->value() * R->value());
	if (L->has_value() and L->value() == 0)
		return std::make_unique<Value>(0);
	if (R->has_value() and R->value() == 0)
		return std::make_unique<Value>(0);
	if (L->has_value() and L->value() == 1)
		return R;
	if (R->has_value() and R->value() == 1)
		return L;
	return std::make_unique<Mul>(std::move(L), std::move(R));
}

std::unique_ptr<Node> Div::Simplify() const
{
	auto L = l->Simplify();
	auto R = r->Simplify();

	if (L->has_value() and R->has_value())
		return std::make_unique<Value>(L->value() * R->value());
	if (L->has_value() and L->value() == 0)
		return std::make_unique<Value>(0);
	if (R->has_value() and R->value() == 0)
		return std::make_unique<Value>(std::numeric_limits<float>::infinity());
	if (R->has_value() and R->value() == 1)
		return L;
	return std::make_unique<Div>(std::move(L), std::move(R));
}

std::unique_ptr<Node> Pow::Simplify() const
{
	auto L = l->Simplify();
	auto R = r->Simplify();

	if (L->has_value() and R->has_value())
		return std::make_unique<Value>(std::pow(L->value(), R->value()));
	if (L->has_value() and L->value() == 0)
		return std::make_unique<Value>(0);
	if (L->has_value() and L->value() == 1)
		return std::make_unique<Value>(1);
	if (R->has_value() and R->value() == 0)
		return std::make_unique<Value>(1);
	if (R->has_value() and R->value() == 1)
		return L;
	return std::make_unique<Pow>(std::move(L), std::move(R));
}

std::unique_ptr<Node> Exp::Simplify() const
{
	auto N = node->Simplify();

	if (N->has_value())
		return std::make_unique<Value>(std::exp(N->value()));
	if (auto tmp = dynamic_cast<Log*>(N.get()))
		return tmp->node->Simplify();
	return std::make_unique<Exp>(std::move(N));
}

std::unique_ptr<Node> Log::Simplify() const
{
	auto N = node->Simplify();

	if (N->has_value())
		return std::make_unique<Value>(std::log(N->value()));
	if (auto tmp = dynamic_cast<Exp*>(N.get()))
		return tmp->node->Simplify();
	return std::make_unique<Log>(std::move(N));
}
