#include <stdio.h>
#include <string.h>

int nr_symbols = 0;
int nr_states = 0;

char tm[10][10][3];

int verbose_level = 0;

bool is_symbol(char c) { return '0' <= c && c < '0' + nr_symbols; }

struct Rule
{
	char symbol;
	Rule *children;
	Rule *next;
	Rule() : children(0), next(0) {}
	void print(bool left) { print(left, stdout); }
	void print(bool left, FILE *fout)
	{
		if (left && next != 0)
			next->print(left, fout);
		if (children != 0)
		{
			if (children->next == 0)
			{
				fprintf(fout, "%c", children->symbol);
			}
			else
			{
				fprintf(fout, "(");
				children->print(left, fout);
				fprintf(fout, ")");
			}
		}
		fprintf(fout, "%c", symbol);
		if (!left && next != 0)
			next->print(left, fout);
	}
};

bool equal(Rule *l, Rule *r)
{
	return    (l == 0 && r == 0)
	       || (   l != 0 && r != 0
	           && l->symbol == r->symbol
	           && equal(l->children, r->children) 
	           && equal(l->next, r->next));
}

struct Pattern
{
	char state;
	Rule *left;
	char head;
	Rule *right;
	int used;
	Pattern *next;
	Pattern() : left(0), right(0), next(0), used(0), kind(' ') {}
	void print()
	{
		printf("%c: ", state);
		left->print(true);
		printf(" %c ", head);
		if (right != 0)
			right->print(false);
		else
			printf("??");
	}
	char kind;	
};

bool parse_error = false;

Rule *parse_rule(char *&s, bool left)
{
	Rule *result = 0;
	Rule **ref_rule = &result;
	while (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n' && *s != '\0' && *s != ')')
	{
		Rule *new_rule = new Rule;
		if (*s == '(')
		{
			s++;
			new_rule->children = parse_rule(s, left);
			if (*s != ')')
			{
				parse_error = true;
				return 0;
			}
			s++;
			if (*s == '+' || *s == '*' || *s == '@')
			{
				new_rule->symbol = *s++;
			}
			else
			{
				parse_error = true;
				return 0;
			}
		}
		else if (*s == '.' || is_symbol(*s))
		{
			if (s[1] == '+' || s[1] == '*' || s[1] == '@')
			{
				new_rule->children = new Rule;
				new_rule->children->symbol = *s++;
				new_rule->children->children = 0;
			}
			new_rule->symbol = *s++;
		}
		if (left)
		{
			new_rule->next = *ref_rule;
			*ref_rule = new_rule;
		}
		else
		{
			*ref_rule = new_rule;
			ref_rule = &(*ref_rule)->next;
		}
	}
	return result;
}

bool matchRulePart(Rule *pattern, bool plus, Rule *target, Rule *&remainder);

bool matchRule(Rule *pattern, Rule *target)
{
	Rule *remainder = 0;
	return matchRulePart(pattern, false, target, remainder) && remainder == 0;
}

#ifdef DEBUG
int depth = 0;

#define ENTER printf("%*.*s", depth, depth,""); depth += 2;
#define BOOLRETURN(X) { bool r = X; depth -= 2; printf("%*.*s= %s\n", depth, depth,"", r ? "true" : "false"); return r; }
#else
#define ENTER
#define BOOLRETURN(X) return X;
#endif

bool matchRulePart(Rule *pattern, bool plus, Rule *target, Rule *&remainder)
{
	ENTER
#ifdef DEBUG
	printf("matchRulePart "); if (pattern) pattern->print(false); printf(" %d ", plus); if (target) target->print(false); printf("\n");
#endif
	remainder = pattern;
	if (target == 0)
		BOOLRETURN(true)
	if (pattern == 0)
	{
		while (target != 0 && target->symbol == '*')
			target = target->next;
		BOOLRETURN(target == 0)
	}

	if (   target->symbol == '@'
		&& target->children->symbol == '.'
		&& target->children->next == 0
		&& target->next == 0)
	{
		remainder = 0;
		BOOLRETURN(true)
	}
	
	char sym = '\0';
	int target_nr = 0;
	bool target_more = false;
	if (is_symbol(target->symbol))
	{
		sym = target->symbol;
		target_nr = 1;
	}
	else if (is_symbol(target->children->symbol) && target->children->next == 0)
	{
		sym = target->children->symbol;
		target_more = true;
		target_nr = target->symbol == '+' ? 1 : 0;
	}
	if (sym != '\0')
	{
		Rule *target2 = target->next;
		for (; target2 != 0; target2 = target2->next)
			if (target2->symbol == sym)
				target_nr++;
			else if (target2->children != 0 && target2->children->symbol == sym && target2->children->next == 0)
			{
				if (target2->symbol == '+')
					target_nr++;
				target_more = true;
			}
			else
				break;
		
		int pattern_nr = 0;
		bool pattern_more = false;
		Rule *pattern2 = pattern;
		for (; pattern2 != 0 && (target_more || pattern_nr < target_nr); pattern2 = pattern2->next)
			if (pattern2->symbol == sym)
				pattern_nr++;
			else if (pattern2->children != 0 && pattern2->children->symbol == sym && pattern2->children->next == 0)
			{
				if (pattern2->symbol == '+')
					pattern_nr++;
				pattern_more = true;
			}
			else
				break;
		if (target_more ? pattern_nr >= target_nr : !pattern_more && pattern_nr == target_nr)
			BOOLRETURN(matchRulePart(pattern2, false, target2, remainder))
	}
	
	bool result = false;
	if (    (   (pattern->symbol == '+' && (target->symbol == '+' || target->symbol == '*'))
	         || (pattern->symbol == '*' && ((plus && target->symbol == '+') || target->symbol == '*')))
	    && matchRule(pattern->children, target->children)
		&& matchRulePart(pattern->next, false, target->next, remainder))
		BOOLRETURN(true)
		
	if (target->symbol == '*' || target->symbol == '+')
	{
		Rule *intermediate = 0;
		if (   matchRulePart(pattern, false, target->children, intermediate)
			&& matchRulePart(intermediate, true, target, remainder))
			BOOLRETURN(true)
	}
	
	if (target->symbol == '*' || (plus && target->symbol == '+'))
		BOOLRETURN(matchRulePart(pattern, false, target->next, remainder))

	BOOLRETURN(false)
}

bool canAbsorb(Rule *head, Rule *target)
{
	ENTER
#ifdef DEBUG
	printf("canAbsorb "); if (head) head->print(false); printf(" | "); if (target) target->print(false); printf("\n");
#endif
	for (; target != 0; target = target->next)
	{
		if (target->symbol == '@')
			BOOLRETURN(matchRule(head, target))
		if (target->symbol != '+' && target->symbol != '*')
			break;
		bool start = false;
		if (   (   (target->children->symbol != '+' && target->children->symbol != '*')
			    || !matchRule(head, target->children->children))
			&& !canAbsorb(head, target->children))
			BOOLRETURN(false)
		if (target->symbol == '+')
			BOOLRETURN(true)
	}
	BOOLRETURN(false)
}

bool matchWholeRule(Rule *pattern, Rule *target)
{
	if (matchRule(pattern, target))
		return true;
	Rule **ref_rest = &pattern;
	int n = 0;
	while ((*ref_rest) != 0 && is_symbol((*ref_rest)->symbol))
	{
		n++;
		ref_rest = &(*ref_rest)->next;
	}
	if (n == 0)
		return false;
	Rule *rest = *ref_rest;
	*ref_rest = 0;
	bool result =    canAbsorb(pattern, target)
				  && matchRule(rest, target);
	*ref_rest = rest;
	return result;

}			

Pattern *patterns = 0;
Pattern **ref_pattern = &patterns;

bool find_matching3(Pattern *pattern, bool nested)
{
	for (Pattern *target = patterns; target != 0; target = target->next)
		if (   pattern->state == target->state
		    && pattern->head == target->head
			&& equal(pattern->left, target->left)
			&& equal(pattern->right, target->right))
		{
			if (verbose_level == 2)
			{
				printf("  (");
				pattern->print();
				printf(" included)\n");
			}	
			target->used++;
			return true;
		}

	for (Pattern *target = patterns; target != 0; target = target->next)
		if (   pattern->state == target->state
		    && pattern->head == target->head
			&& matchWholeRule(pattern->left, target->left)
			&& matchWholeRule(pattern->right, target->right))
		{
			if (verbose_level >= 1)
			{
				printf("  (");
				pattern->print();
				printf(" matches ");
				target->print();
				printf(")\n");
			}
			target->used++;
			return true;
		}
	
	if (pattern->left->symbol == '*')
	{
		pattern->left->symbol = '+';
		bool result = find_matching3(pattern, true);
		pattern->left->symbol = '*';
		if (result)
		{
			Pattern pattern2 = *pattern;
			pattern2.left = pattern->left->next;
			if (find_matching3(&pattern2, true));
				return true;
		}
	}
	
	if (pattern->right->symbol == '*')
	{
		pattern->right->symbol = '+';
		bool result = find_matching3(pattern, true);
		pattern->right->symbol = '*';
		if (result)
		{
			Pattern pattern2 = *pattern;
			pattern2.right = pattern->right->next;
			if (find_matching3(&pattern2, true));
				return true;
		}
	}

	if (!nested)
	{
		printf("  (no match: ");
		pattern->print();
		printf(")\n");
	
		*ref_pattern = new Pattern();
		(*ref_pattern)->state = pattern->state;
		(*ref_pattern)->left = pattern->left;
		(*ref_pattern)->head = pattern->head;
		(*ref_pattern)->right = pattern->right;
		(*ref_pattern)->used = 1;
		(*ref_pattern)->kind = 'n';
		ref_pattern = &(*ref_pattern)->next;
	}
	
	return false;
}

bool find_matching2(Pattern *pattern)
{
	if (find_matching3(pattern, false))
		return true;
	
	Pattern pattern2 = *pattern;
	while (pattern2.right->symbol == '*')
	{
		pattern2.right->symbol = '+';
		bool found = find_matching3(&pattern2, false);
		pattern2.right->symbol = '*';
		if (!found)
		{
			printf("Error: did not find ");
			pattern2.print();
			printf("\n");
			break;
		}
		pattern2.right = pattern2.right->next;
		if (find_matching3(&pattern2, false))
			return true;
	}
	return false;
}

bool find_matching(Pattern *pattern)
{
	if (pattern->head == '.')
	{
		bool result = true;
		for (char s = '0'; s < '0' + nr_symbols; s++)
		{
			pattern->head = s;
			if (!find_matching2(pattern))
				result = false;
		}
		pattern->head = '.';
		return result;
	}
	return find_matching2(pattern);
}

void process_pattern(Pattern *pattern, Rule *new_pull, Rule *new_push, bool move_right, char head, const char *tr)
{
	Pattern *np = new Pattern;
	np->state = tr[2];
	np->head = head;
	if (move_right)
	{
		np->right = new_pull;
		np->left = new_push;
	}
	else
	{
		np->left = new_pull;
		np->right = new_push;
	}
	
	pattern->print();
	printf(" (%c%c%c) ", tr[0], tr[1], tr[2]);
	printf("=> ");
	np->print();
	printf("\n");
	if (!find_matching(np))
	{
		printf("Error: no matching patterns\n");
	}
}

void expand_pattern(Pattern *pattern, Rule *pull_rule, Rule *new_push, bool move_right, const char *tr)
{
	if (pull_rule->symbol == '@')
	{
		process_pattern(pattern, pull_rule, new_push, move_right, pull_rule->children->symbol, tr);
	}
	else if (pull_rule->symbol == '+' || pull_rule->symbol == '*')
	{
		Rule *new_pull = 0;
		Rule **ref_new = &new_pull;
		Rule *rule = pull_rule->children;
		for (; rule != 0; rule = rule->next)
		{
			*ref_new = new Rule;
			(*ref_new)->symbol = rule->symbol;
			(*ref_new)->children = rule->children;
			ref_new = &(*ref_new)->next;
		}
		*ref_new = new Rule;
		(*ref_new)->symbol = '*';
		(*ref_new)->children = pull_rule->children;
		(*ref_new)->next = pull_rule->next;
		expand_pattern(pattern, new_pull, new_push, move_right, tr);
		if (pull_rule->symbol == '*')
			expand_pattern(pattern, pull_rule->next, new_push, move_right, tr);
	}
	else
	{
		process_pattern(pattern, pull_rule->next, new_push, move_right, pull_rule->symbol, tr);
	}
}

void unit_tests();

int main(int argc, char *argv[])
{
	unit_tests();

#ifdef DEBUG
	return 0;
#endif

	const char *fn = 0;
	for (int i = 1; i < argc; i++)
		if (strcmp(argv[i], "-v") == 0)
			verbose_level = 1;
		else if (strcmp(argv[i], "-vv") == 0)
			verbose_level = 2;
		else if (i + 1 < argc && strcmp(argv[i], "-a") == 0)
			fn = argv[++i];
	
	FILE *fin = stdin;
	if (fn != 0)
	{
		fin = fopen(fn, "r");
		if (fin == 0)
		{
			printf("Cannot open '%s'\n", fn);
			return 0;
		}
	}
	
	char buffer[100];
	
	fgets(buffer, 100, fin);
	if (strchr(buffer, '_') != 0)
	{
		int i = 0;
		char *s = buffer;
		while (*s > ' ')
		{
			if (*s == '_')
			{
				s++;
				i = 0;
				nr_states++;
			}
			else
			{
				for (int j = 0; j < 3 && *s != '\n' && *s != '\0'; j++)
					tm[nr_states][i][j] = *s++;
				i++;
				if (i > nr_symbols)
					nr_symbols = i;
			}
		}
		nr_states++;
	}
	else
	{
		for (char *s = buffer; *s != '\n' && *s != '\0'; s++)
			if (*s == '0' + nr_symbols)
				nr_symbols++;
		
		while (fgets(buffer, 100, fin))
		{
			char *s = buffer;
			while (*s == ' ' || *s == '\t') s++;
			if (*s != 'A' + nr_states)
				break;
			s++;
			for (int i = 0; i < nr_symbols; i++)
			{
				while (*s == ' ' || *s == '\t') s++;
				for (int j = 0; j < 3 && *s != '\n' && *s != '\0'; j++)
					tm[nr_states][i][j] = *s++;
			}
			nr_states++;
		}
	}
	
	printf("  ");
	for (int i = 0; i < nr_symbols; i++)
		printf("   %c", '0' + i);
	printf("\n");
	for (int i = 0; i < nr_states; i++)
	{
		printf("%c ", 'A' + i);
		for (int j = 0; j < nr_symbols; j++)
			printf(" %c%c%c", tm[i][j][0], tm[i][j][1], tm[i][j][2]);
		printf("\n");
	}
	printf("\n");


	while (fgets(buffer, 100, fin))
	{
		char *s = buffer;
		while (*s == ' ' || *s == '\t') s++;
		if ('A' <= *s && *s < 'A' + nr_states)
		{
			Pattern *pattern = new Pattern;
			pattern->state = *s++;
			if (*s != ':')
			{
				printf("Error: missing colon\n");
				continue;
			}
			s++;
			
			parse_error = false;
			while (*s == ' ' || *s == '\t') s++;
			pattern->left = parse_rule(s, true);
			while (*s == ' ' || *s == '\t') s++;
			if (is_symbol(*s))
				pattern->head = *s++;
			else
			{
				printf("Error in head %s\n", buffer);
				continue;
			}
			while (*s == ' ' || *s == '\t') s++;
			pattern->right = parse_rule(s, false);
			
			if (parse_error)
			{
				printf("Error in %s\n", buffer);
				continue;
			}
			
			(*ref_pattern) = pattern;
			ref_pattern = &pattern->next;
		}
	}
	
	if (fn != 0)
		fclose(fin);

	for (Pattern *pattern1 = patterns; pattern1 != 0; pattern1 = pattern1->next)
	{
		bool before = false;
		for (Pattern *pattern2 = patterns; pattern2 != 0; pattern2 = pattern2->next)
			if (pattern1 == pattern2)
				before = true;
			else if (   pattern1->state == pattern2->state
					 && pattern1->head == pattern2->head
					 && matchRule(pattern1->left, pattern2->left)
					 && matchRule(pattern1->right, pattern2->right))
			{
				if (   equal(pattern1->left, pattern2->left)
					&& equal(pattern1->right, pattern2->right))
				{
					if (!before)
					{
						printf("Error: pattern ");
						pattern1->print();
						printf(" is repeated\n");
						pattern2->kind = '=';
					}
				}
				else
				{
					printf("Error: pattern is ");
					pattern1->print();
					printf(" matches ");
					pattern2->print();
					printf("\n");
					pattern1->kind = '<';
				}
				break;
			}
	}
	
	for (Pattern *pattern = patterns; pattern != 0; pattern = pattern->next)
	{
		if (pattern->kind != ' ')
			continue;
			
		int state = pattern->state - 'A';
		int symbol = pattern->head - '0';
		const char *tr = tm[state][symbol];
		if (tr[0] == '-') continue;
		
		bool move_right = tr[1] == 'R';
		Rule *pull_rule, *push_rule;
		if (move_right)
		{
			pull_rule = pattern->right;
			push_rule = pattern->left;
		}
		else
		{
			pull_rule = pattern->left;
			push_rule = pattern->right;
		}
		
		Rule *new_push = new Rule;
		new_push->symbol = tr[0];
		new_push->next = push_rule;
		
		expand_pattern(pattern, pull_rule, new_push, move_right, tr);
	}
	
	printf("\n\n");
	for (Pattern *pattern = patterns; pattern != 0; pattern = pattern->next)
		if (   pattern->used == 0
			&& !(   pattern->state == 'A'
				 && pattern->left->symbol == '@'
				 && pattern->left->next == 0
				 && pattern->left->children->symbol == '0'
				 && pattern->left->children->next == 0
				 && pattern->head == '0'
				 && pattern->right->symbol == '@'
				 && pattern->right->next == 0
				 && pattern->right->children->symbol == '0'
				 && pattern->right->children->next == 0))
		{
			printf("Warning: Unused rule: ");
			pattern->print();
			printf("\n");
		}
	
	if (fn != 0)
	{
		FILE *fout = fopen(fn, "w");
		
		for (int i = 0; i < nr_states; i++)
		{
			if (i > 0)
				fprintf(fout, "_");
			for (int j = 0; j < nr_symbols; j++)
				fprintf(fout, "%c%c%c", tm[i][j][0], tm[i][j][1], tm[i][j][2]);
		}
		fprintf(fout, "\n\n");
		
		bool new_patterns = false;
		
		for (Pattern *pattern = patterns; pattern != 0; pattern = pattern->next)
		{
			if (pattern->kind == '=')
				fprintf(fout, "/ Repeated ");
			else if (pattern->kind == '<')
				fprintf(fout, "/ Matching ");
			else if (pattern->kind == 'n' && !new_patterns)
			{
				fprintf(fout, "\n/ New:\n");
				new_patterns = true;
			}
			fprintf(fout, "%c: ", pattern->state);
			pattern->left->print(true, fout);
			fprintf(fout, " %c ", pattern->head);
			pattern->right->print(false, fout);
			fprintf(fout, "\n");
		}
		fclose(fout);
	}
	
	return 0;
}


void rule_match_test(const char *name, const char *p, const char *t, int nr, bool left, bool match)
{
#ifdef DEBUG
	printf("\nTest %s\n", name);
#endif
	int old_nr = nr_symbols;
	nr_symbols = nr;
	char buf[100];
	strcpy(buf, p);
	char *s = buf;
	Rule *pattern = parse_rule(s, left);
	strcpy(buf, t);
	s = buf;
	Rule *target = parse_rule(s, left);
	if (matchWholeRule(pattern, target) != match)
	{
		printf("Error: unit test %s failed. ", name);
		pattern->print(left); printf(" does %smatch ", match ? "not " : "");
		target->print(left); printf("\n");
	}
	nr_symbols = old_nr;
}

void unit_tests()
{
	rule_match_test("test1", "01(01)+1@", "(01)+1@", 2, false, true);
	rule_match_test("test2", "01(01)*1@", "(01)+1@", 2, false, true);
	rule_match_test("test3", "01(01)*.@", "(01)*.@", 2, false, true);
	rule_match_test("test4", "01(010*)+1@", "(010*)+1@", 2, false, true);
	rule_match_test("test5", "01(1*01)+1@", "(1*01)+1@", 2, false, true);
	rule_match_test("test6", "01(01+)*.@", "(01+)*.@", 2, false, true);
	rule_match_test("test7", "00*1@", "0*1@", 2, false, true);
	rule_match_test("test8", "01(01)*2@", "(01)*2@", 3, false, true);
	rule_match_test("test9", "01(01)+2@", "(01)+2@", 3, false, true);
	rule_match_test("test10", "0@(10(00)*)*00", ".@(10(00)*)*", 2, true, true);
	rule_match_test("test11", "0@(10(00)*)+10", "0@(10(00)*)+", 2, true, true);
	rule_match_test("test12", "0@(0*10(00)*)+10", "0@(0*10(00)*)+", 2, true, true);

	//  Match not found #2  E: 0@10 1 101101(001)*0@" despite "E: (0)@(10)* 1 1(01)*101(001)*(0)@
	rule_match_test("Issue#2", "0@10", "(0)@(10)*", 2, true, true);
	rule_match_test("Issue#2", "101101(001)*0@", "1(01)*101(001)*(0)@", 2, false, true);
	
	// canAbsorb tests
	rule_match_test("canAbsorb1", "01(1+01)+1@", "(1+01)+1@", 2, false, false);
	rule_match_test("canAbsorb2", "10(01)*2@", "(01)*2@", 3, false, false);
	rule_match_test("canAbsorb3", "0@(100*)*0", "0@(100*)*", 2, true, true);
	rule_match_test("canAbsorb4", "0@(10(00)*)*00", "0@(10(00)*)*", 2, true, true);
	rule_match_test("canAbsorb5", "1@(10(00)*)*00", "1@(10(00)*)*", 2, true, false);
	rule_match_test("canAbsorb6", "1@(10(00)*)+00", "1@(10(00)*)+", 2, true, true);
	rule_match_test("canAbsort7", "00(((00)*1)+1)+1@", "(((00)*1)+1)+1@", 2, false, true);
	rule_match_test("canAbsort8", "00(((00)*1)*((00)*22)+1)+1@", "(((00)*1)*((00)*22)+1)+1@", 3, false, true);
	rule_match_test("canAbsort9", "00(((002*)*1)*((1*00)*22)+1)+1@", "(((002*)*1)*((1*00)*22)+1)+1@", 3, false, true);
	rule_match_test("canAbsort10", "00(((002*)*1)*((1*00)*22)*1)+1@", "(((002*)*1)*((1*00)*22)*1)+1@", 3, false, false);
}
