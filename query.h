#pragma once

#include <memory>

#include <QString>
#include <QSet>
#include <QVector>
#include <QMap>

/*
	Simple Query language
	- Retreives a set of media id by performing set operations
	- Supports UNION, INTERSECTION, DIFFERENCE set operations
	- Supports parenthesis for specify order of operation
	- Supports quotation to denote tagnames containing reserved syntax
	- Default order of operation left to right
	- All operators have the same precedence
*/

enum NodeType {
	TAG,
	UNION,
	INTERSECT,
	DIFF,
	OPEN_PAREN,
	CLOSE_PAREN
};

struct ASTNode {
	
	NodeType type;
	unsigned int close_paren_idx;
	QSet<unsigned int> result;
	
	std::shared_ptr<ASTNode> left;
	std::shared_ptr<ASTNode> right;
};

struct AST {
	std::shared_ptr<ASTNode> root = nullptr;
};

class Query
{
public:

	QString raw_str;
	AST ast;
	QVector<std::shared_ptr<ASTNode>> token_vec;
	QSet<unsigned int> result;
	//QMap<QString, QSet<unsigned int>> tag_name_to_tag_media_id_map;

	explicit Query(const QString&);
	~Query();

	int Tokenize( std::function< int(const QString& tag_name, QSet<unsigned int>* out) > get_tag_media_list_handler );
	int	GenerateAST();
	int ProcessAST();

	//int GetTagNameList(QList<QString>*);
	//int InsertTagMediaIds(const QString&, const QSet<unsigned int>&);

private:

	int ProcessASTRecur(std::shared_ptr<ASTNode>&);
	int GenerateASTRecur(int begin, int end, std::shared_ptr<ASTNode>*);
};

