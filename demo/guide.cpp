
input:
	vector<lexToken> lexTokens;
	set<string> terminalTokens;
	set<string> nonTerminalTokens;
	vector<Prod> prods;

output: 


@variable
const int num_lexical_tokens = 2;
const int num_syntax_tokens = 3;

auto ending = Token::Terminator("EOF_FLAG");

@variable
#define AUTO 256
#define primary_expression 257
#define postfix_expression 258


// lexical
@variable
vector<RegEx> lexical_tokens_regex = {
	"auto", 
	"\\+"
}
@variable
vector<Token::Terminator> lexical_tokens = {
	Token::Terminator("auto"), 
	Token::Terminator("+"), 
}

// lexical to syntax
@variable
IdType to_syntax_token_id(
		Token::Terminator lexical_token, 
		AttrDict ad) {
	swtich (leixcal_token.GetTokName) {
		case ("auto"):
			{ count(); return(AUTO); }
			break;
		case ("+"):
			{ count(); return('+'); }
			break;
	}
}

auto [transition, state] = DfaModel::Merge(lexical_tokens_regex)
auto s2ppl = Stream2TokenPipe(transition, state, lexical_tokens, ending);

// syntax
Token syntax_tokens[256 + num_syntax_tokens] = {
	Token::Terminator(string(1, 0)),				// 0
	Token::Terminator(string(1, 1)), 
	Token::Terminator(string(1, 2)), 
	Token::Terminator(string(1, 3)), 
	...
	Token::Terminator(string(1, 255), ), 			// 255 
	Token::Terminator("AUTO", ), 					// 256
	Token::NonTerminator("primary_expression", ), 	// 257
	Token::NonTerminator("postfix_expression", ), 	// 258
};

auto start_syntax_token = syntax_tokens[primary_expression]

@variable
vector<Production> productions = {
	// primary_expression : AUTO postfix_expression ; 
	Production(syntax_tokens[primary_expression], {[](vector<YYSTATE> &v) {
    	// ...
	}})(syntax_tokens[AUTO])(syntax_tokens[postfix_expression]), 
}


// syntax
ContextFreeGrammar cfg(productions, start_syntax_token, ending);
Lr1 lr1;
cfg.Compile(lr1);
auto table = cfg.GetLrTable();
LrParser parser(table);


// runtime
while (true) {
	auto lexical_token = s2ppl.Defer(input_stream);
	AttrDict ad;
	ad.Set("lval", s2ppl.buffer_); 

	IdType id = to_syntax_token_id(lexical_token, ad)
	Token syntax_token = syntax_tokens[id]

	tokens.emplace_back(syntax_token);
	attributes.emplace_back(ad);
	if (lexical_token == ending)
		break;
}

parser.Parse(tokens, attributes);
auto tree = parser.GetTree();






