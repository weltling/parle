<?php

namespace Parle;

use Parle\{Parser, ParserException, Lexer, Token, Stack};

/* As in the PHP manual, strings like first=value&arr[]=foo+bar&arr[]=baz */
/* Native urldecode, array and some other helper implementations used. */

class ParseStrParser extends Parser
{
	protected $lex;
	protected $stack;
	protected $tokenNameToId = array();
	protected $tokenIdToName = array();
	protected $prodHandler = array();
	protected $result = array();
	protected $debug = false;

	public function __construct(Lexer $lex, bool $debug = false)
	{
		$this->lex = $lex;
		$this->stack = new Stack;
		$this->debug = $debug;
	}

	public function init()
	{
		$this->terminal("left", "'='", "[=]");
		$this->terminal("token", "']'", "[\]]");
		$this->terminal("right", "'['", "[\[]");
		$this->terminal("left", "'&'", "[&]");
		$this->terminal("token", "T_STR", "[^=\[\]&\s]+");

		$this->production("START", "PAIRS");
		$this->production("PAIRS", "PAIR");
		$this->production("PAIRS", "PAIRS '&' PAIR");
		$this->production("VALUE", "");
		$this->production("VALUE", "T_STR");
		$this->production("ARRKEY", "", "handleEmptyDimensionKey");
		$this->production("ARRKEY", "T_STR", "handleDimensionKey");
		$this->production("ARRDIM", "'[' ARRKEY ']'");
		$this->production("ARRDIM", "'[' ARRKEY ']' ARRDIM");
		$this->production("PAIR", "T_STR ARRDIM '=' VALUE", "handleArray");
		$this->production("PAIR", "T_STR '=' VALUE", "handleScalar");

		$this->build();
		$this->lex->build();
	}

	protected function terminal(string $assoc, string $sym, string $reg)
	{
		switch ($assoc) {
		default:
			throw new ParserException("Unknown associativity '$assoc'.");
		case "left":
			$this->left($sym);
			break;
		case "right":
			$this->right($sym);
			break;
		case "token":
			$this->token($sym);
			break;
		case "nonassoc":
			$this->nonassoc($sym);
			break;
		}

		$id = $this->tokenId($sym);
		$this->lex->push($reg, $id);

		$this->tokenNameToId[$sym] = $id;
		$this->tokenIdToName[$id] = $sym;
	}

	protected function production(string $name, string $rule, $handler = NULL)
	{
		$id = $this->push($name, $rule);
		if ($handler) {
			$this->prodHandler[$id] = array($this, $handler);
		}
	}

	private function handleEmptyDimensionKey()
	{
		$this->stack->push(NULL);
	}

	private function handleDimensionKey()
	{
		$this->stack->push($this->sigil());
	}

	private function handleScalar()
	{
		$name = $this->sigil();
		$val = $this->sigil(2);
		$this->result[$name] = urldecode($val);
	}

	private function handleArray()
	{
		$name = $this->sigil();
		$val = $this->sigil(3);

		// create top array element
		$k = $this->stack->top;
		$tmp = array();
		if ($k) {
			$tmp[$k] = urldecode($val);
		} else {
			$tmp[] = urldecode($val);
		}
		$this->stack->pop();

		// check if there are more dimensions
		while (!$this->stack->empty) {
			$k = $this->stack->top;
			$tmp2 = array();
			if ($k) {
				$tmp2[$k] = $tmp;
			} else {
				$tmp2[] = $tmp;
			}
			$this->stack->pop();
			$tmp = $tmp2;
		}
		if (!array_key_exists($name, $this->result)) {
			$this->result[$name] = array();
		}
		$this->result[$name] = array_merge_recursive($this->result[$name], $tmp);
	}

	public function parse($in)
	{
		$this->result = array();
		$this->stack = new Stack;

		$this->consume($in, $this->lex);

		while (Parser::ACTION_ACCEPT != $this->action) {
			switch ($this->action) {
				case Parser::ACTION_ERROR:
					$i = $this->errorInfo();
					switch ($i->id) {
						case Parser::ERROR_SYNTAX:
							throw new ParserException("Syntax error at " . $i->position);
						case Parser::ERROR_NON_ASSOCIATIVE:
							throw new ParserException("Token " . $this->tokenIdToName[$i->token->id] . "is not associative");
						case Parser::ERROR_UNKNOWN_TOKEN:
							throw new ParserException("Unknown token '" . $i->token->value . "' at " . $i->position);
					}
					break;
				case Parser::ACTION_SHIFT:
				case Parser::ACTION_GOTO:
					if ($this->debug) {
						echo $this->trace(), PHP_EOL;
					}
					break;
				case Parser::ACTION_REDUCE:
					if ($this->debug) {
						echo $this->trace(), PHP_EOL;
					}
					if (array_key_exists($this->reduceId, $this->prodHandler)) {
						if ($this->debug) {
							echo "calling ", $this->prodHandler[$this->reduceId][1], PHP_EOL;
						}
						call_user_func($this->prodHandler[$this->reduceId]);
					}
					break;
			}
			$this->advance();
		}

		return $this->result;
	}
}

function parse_str(string $in, array &$result = array())
{
	$p = new ParseStrParser(new Lexer);

	$p->init();

	$result = $p->parse($in);
}
