#pragma once
#include <string>
#include <memory>
#include <ostream>
#include <vector>

// Forward declaration
class SymExpVec;
class Var;
namespace AST
{
	class Node;
}

// Wrapper to provide value semantics
class SymExp
{
	std::unique_ptr<AST::Node> root;
protected:
	explicit SymExp(std::unique_ptr<AST::Node>&& node) noexcept : root(std::move(node)) {}
public:
	SymExp() = delete;
	explicit SymExp(float value) noexcept;
	explicit SymExp(std::string name) noexcept;

	SymExp(const SymExp&) noexcept;
	SymExp(SymExp&&) noexcept = default;
	SymExp& operator=(const SymExp&) noexcept;
	SymExp& operator=(SymExp&&) noexcept = default;
	~SymExp() = default;

	[[nodiscard]] SymExp At(const Var&, float value) const;
	[[nodiscard]] SymExp At(const std::vector<Var>&, const std::vector<float>& values) const;
	[[nodiscard]] SymExp Derive(const Var&) const;
	[[nodiscard]] SymExpVec Derive(const std::vector<Var>&) const;
	[[nodiscard]] SymExpVec DeriveAt(const std::vector<Var>&, const std::vector<float>& values) const;
	[[nodiscard]] SymExp Simplify() const;
	[[nodiscard]] std::string to_string() const;

	[[nodiscard]] bool has_value() const;
	[[nodiscard]] float value() const;

	friend SymExp operator-(SymExp);
	friend SymExp operator+(SymExp, SymExp);
	friend SymExp operator-(SymExp, SymExp);
	friend SymExp operator*(SymExp, SymExp);
	friend SymExp operator/(SymExp, SymExp);
	friend SymExp pow(SymExp, SymExp);
	friend SymExp exp(SymExp);
	friend SymExp log(SymExp);
};

class SymExpVec
{
	std::vector<SymExp> vec;
public:
	SymExpVec(std::vector<SymExp> vec) noexcept : vec(std::move(vec)) {}

	[[nodiscard]] SymExpVec At(const Var&, float value) const;
	[[nodiscard]] SymExpVec At(const std::vector<Var>&, const std::vector<float>& values) const;

	[[nodiscard]] auto begin() noexcept { return vec.begin(); }
	[[nodiscard]] auto begin() const noexcept { return vec.begin(); }
	[[nodiscard]] auto cbegin() const noexcept { return vec.cbegin(); }
	[[nodiscard]] auto end() noexcept { return vec.end(); }
	[[nodiscard]] auto end() const noexcept { return vec.end(); }
	[[nodiscard]] auto cend() const noexcept { return vec.cend(); }

	[[nodiscard]] std::vector<float> value() const;

	operator std::vector<SymExp>() { return vec; }
};

class Var final : public SymExp
{
public:
	Var() noexcept;
	Var(std::string name) noexcept;
	Var(float value) noexcept;
};

[[nodiscard]] inline std::string to_string(const SymExp& se) { return se.to_string(); }
[[nodiscard]] inline std::ostream& operator<<(std::ostream& os, const SymExp& se) { return os << to_string(se); }


namespace AST
{
	// Forward declaration
	class Value;
	class Symbol;
	class Neg;
	class Add;
	class Sub;
	class Mul;
	class Div;
	class Pow;
	class Exp;
	class Log;

	// Interface
	class Node
	{
	public:
		[[nodiscard]] virtual std::unique_ptr<Node> Clone() const noexcept = 0;
		[[nodiscard]] virtual std::unique_ptr<Node> Eval(const Symbol&, float value) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Node> Derive(const Symbol&) const = 0;
		[[nodiscard]] virtual std::unique_ptr<Node> Simplify() const = 0;
		[[nodiscard]] virtual std::string to_string() const = 0;

		[[nodiscard]] virtual bool has_value() const { return false; }
		[[nodiscard]] virtual float value() const { throw; }
	};

	class Value final : public Node
	{
		float val;
	public:
		Value(float value) noexcept : val(value) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Value>(val); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override { return Clone(); }
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol&) const override { return std::make_unique<Value>(0); }
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override { return Clone(); }
		[[nodiscard]] std::string to_string() const override { return std::to_string(val); }

		[[nodiscard]] bool has_value() const override { return true; }
		[[nodiscard]] float value() const override { return val; }
	};

	class Symbol final : public Node
	{
		static int counter;
		std::string name;
	public:
		Symbol() noexcept : name('$' + std::to_string(counter++)) {}
		Symbol(std::string name) noexcept : name(std::move(name)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Symbol>(name); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol& s, float value) const override { return s.name == name ? std::make_unique<Value>(value) : Clone(); }
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override { return std::make_unique<Value>(s.name == name ? 1 : 0); }
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override { return Clone(); }
		[[nodiscard]] std::string to_string() const override { return name; }
	};

	class Neg final : public Node
	{
		std::unique_ptr<Node> node;
	public:
		Neg(std::unique_ptr<Node>&& node) noexcept : node(std::move(node)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Neg>(node->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "-(" + node->to_string() + ")"; }
	};

	class Add final : public Node
	{
		std::unique_ptr<Node> l, r;
	public:
		Add(std::unique_ptr<Node>&& l, std::unique_ptr<Node>&& r) noexcept : l(std::move(l)), r(std::move(r)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Add>(l->Clone(), r->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "(" + l->to_string() + " + " + r->to_string() + ")"; }
	};

	class Sub final : public Node
	{
		std::unique_ptr<Node> l, r;
	public:
		Sub(std::unique_ptr<Node>&& l, std::unique_ptr<Node>&& r) noexcept : l(std::move(l)), r(std::move(r)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Sub>(l->Clone(), r->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "(" + l->to_string() + " - " + r->to_string() + ")"; }
	};

	class Mul final : public Node
	{
		std::unique_ptr<Node> l, r;
	public:
		Mul(std::unique_ptr<Node>&& l, std::unique_ptr<Node>&& r) noexcept : l(std::move(l)), r(std::move(r)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Mul>(l->Clone(), r->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "(" + l->to_string() + " * " + r->to_string() + ")"; }
	};

	class Div final : public Node
	{
		std::unique_ptr<Node> l, r;
	public:
		Div(std::unique_ptr<Node>&& l, std::unique_ptr<Node>&& r) noexcept : l(std::move(l)), r(std::move(r)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Div>(l->Clone(), r->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "(" + l->to_string() + " / " + r->to_string() + ")"; }
	};

	class Pow final : public Node
	{
		std::unique_ptr<Node> l, r;
	public:
		Pow(std::unique_ptr<Node>&& l, std::unique_ptr<Node>&& r) noexcept : l(std::move(l)), r(std::move(r)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Pow>(l->Clone(), r->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "pow(" + l->to_string() + ", " + r->to_string() + ")"; }
	};

	class Exp final : public Node
	{
		friend class Log;
		std::unique_ptr<Node> node;
	public:
		Exp(std::unique_ptr<Node>&& node) noexcept : node(std::move(node)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Exp>(node->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "exp(" + node->to_string() + ")"; }
	};

	class Log final : public Node
	{
		friend class Exp;
		std::unique_ptr<Node> node;
	public:
		Log(std::unique_ptr<Node>&& node) noexcept : node(std::move(node)) {}

		[[nodiscard]] std::unique_ptr<Node> Clone() const noexcept override { return std::make_unique<Log>(node->Clone()); }
		[[nodiscard]] std::unique_ptr<Node> Eval(const Symbol&, float value) const override;
		[[nodiscard]] std::unique_ptr<Node> Derive(const Symbol& s) const override;
		[[nodiscard]] std::unique_ptr<Node> Simplify() const override;
		[[nodiscard]] std::string to_string() const override { return "log(" + node->to_string() + ")"; }
	};
}