#include "query.h"

#include <QStack>
#include <QDebug>

Query::Query(const QString& q) :
	raw_str(q)
{
}


Query::~Query()
{
}

//public


//tokenize() can detect mismatching number of parenthesis
int
Query::Tokenize(std::function< int(const QString& tag_name, QSet<unsigned int>* out) > get_tag_media_list_handler) {
	token_vec.clear();

	QChar curr_char;
	QString tag_name_buff;
	std::shared_ptr<ASTNode> token_buff;
	QStack<int> open_paren_stack;

	bool quote_override_mode = false;

	for (int i = 0; i < raw_str.size(); i++) {

		curr_char = raw_str[i];

		//TODO: convert these if to switch one day

		if (curr_char == '\'' || curr_char == '\"') {
			quote_override_mode = !quote_override_mode;
			continue;
		}

		if (quote_override_mode) {
			tag_name_buff.push_back(curr_char);
			continue;
		}

		//if not part of tag name, ignore empty space
		if (curr_char == ' ' || curr_char == '\t') {
			continue;
		}

		if (curr_char == '+' || curr_char == '*' || curr_char == '-' || curr_char == '(' || curr_char == ')') {
			if (tag_name_buff.size() > 0) {

				token_buff = std::make_shared<ASTNode>();

				token_buff->type = TAG;
				//token_buff->tag_name = tag_name_buff;

				if (get_tag_media_list_handler(tag_name_buff, &(token_buff->result)) < 0) {
					return -1;
				}

				token_vec.push_back(token_buff);
				//tag_name_to_tag_media_id_map.insert(tag_name_buff, QSet<unsigned int>());
				tag_name_buff.clear();
			}

			token_buff = std::make_shared<ASTNode>();

			if (curr_char == '+') {
				token_buff->type = UNION;
			}
			else if (curr_char == '*') {
				token_buff->type = INTERSECT;
			}
			else if (curr_char == '-') {
				token_buff->type = DIFF;
			}
			else if (curr_char == '(') {
				token_buff->type = OPEN_PAREN;
				open_paren_stack.push(token_vec.size());
			}
			else if (curr_char == ')') {
				token_buff->type = CLOSE_PAREN;
				if (open_paren_stack.isEmpty()) {
					//mismatch number of parenthesis
					//more close paren than open
					return -1;
				}

				token_vec[open_paren_stack.pop()]->close_paren_idx = token_vec.size();
			}

			//token_buff.tag_name.clear();
			token_vec.push_back(token_buff);
			continue;
		}

		tag_name_buff.push_back(curr_char);
	}

	if (tag_name_buff.size() > 0) {

		token_buff = std::make_shared<ASTNode>();

		token_buff->type = TAG;
		//token_buff->tag_name = tag_name_buff;

		if (get_tag_media_list_handler(tag_name_buff, &(token_buff->result)) < 0) {
			return -1;
		}

		token_vec.push_back(token_buff);
		//tag_name_to_tag_media_id_map.insert(tag_name_buff, QSet<unsigned int>());
	}

	if (!open_paren_stack.isEmpty()) {
		//mismatch number of parenthesis
		//more open paren than close
		return -1;
	}

	return 1;
}

int 
Query::GenerateAST() {

	GenerateASTRecur(0, token_vec.size(), &ast.root);

	return 1;
}

//in order traverse no recursion
int
Query::ProcessAST() {
	ProcessASTRecur(ast.root);
	result = std::move(ast.root->result);
	return 1;
}

/*
int
Query::GetTagNameList(QList<QString> *out) {

	out->reserve(tag_name_to_tag_media_id_map.size());
	*out = std::move(tag_name_to_tag_media_id_map.keys());
	return 1;
}

int
Query::InsertTagMediaIds(const QString& tag_name, const QSet<unsigned int>& media_id_set) {

	tag_name_to_tag_media_id_map.insert(tag_name, media_id_set);
	return 1;
}
*/

//private

int
Query::ProcessASTRecur(std::shared_ptr<ASTNode>& node) {
	if (node->left != nullptr) {
		ProcessASTRecur(node->left);
	}

	if (node->right != nullptr) {
		ProcessASTRecur(node->right);
	}


	
	switch (node->type) {
	case TAG:
		//do nothing
		return 1;
	case UNION:
		node->result = node->left->result.unite(node->right->result);
		break;
	case INTERSECT:
		node->result = node->left->result.intersect(node->right->result);
		break;
	case DIFF:
		node->result = node->left->result.subtract(node->right->result);
		break;
	default:
		return -1;
	}

	//free up resources from previous operation
	node->left->result.clear();
	node->right->result.clear();

	return 1;
}

int 
Query::GenerateASTRecur(int begin, int end, std::shared_ptr<ASTNode>* out_node) {
	
	std::shared_ptr<ASTNode> local_root = nullptr;
	std::shared_ptr<ASTNode> curr_node;

	for (int i = begin; i < end; i++) {

		curr_node = token_vec[i];

		if (curr_node->type == OPEN_PAREN) {

			int close_paren_idx = curr_node->close_paren_idx;

			GenerateASTRecur(i + 1, curr_node->close_paren_idx, &curr_node);

			if (local_root == nullptr) {
				local_root = curr_node;
			}
			else {
				local_root->right = curr_node;
			}

			//jump over the parenthesis portion
			i = close_paren_idx;

			continue;
		}

		if (curr_node->type == TAG) {

			if (local_root == nullptr) {
				local_root = curr_node;
			}
			else {
				local_root->right = curr_node;
			}

			continue;
		}

		if (curr_node->type == UNION || curr_node->type == INTERSECT || curr_node->type == DIFF) {

			curr_node->left = local_root;
			local_root = curr_node;
			continue;
		}
	}

	*out_node = local_root;
	return 1;
}