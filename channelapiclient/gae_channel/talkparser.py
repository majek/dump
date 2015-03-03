""" Parses array messages as returned by GAE Channel service"""
import re

class ParseError(StandardError):
    pass

_dquote_search = re.compile('(?<!\\\)"')
_squote_search = re.compile('(?<!\\\)\'')
_number_matcher = re.compile('(-?\d+)\.?(\d+)?')
_digit_strings = [str(i) for i in range(10)] + ['-']

_searchers = {'"': _dquote_search,
              "'": _squote_search}

def _match_str(raw_str, pos=0):
    """ searches for a repr() representation of a string starting at pos
    returns the string or None if no string is found
    """
    first_char = raw_str[pos]
    if _searchers.has_key(first_char):
        searcher = _searchers[first_char]
        match = searcher.search(raw_str, pos=pos + 1)
        if not match:
            raise ParseError('unterminated string')
        return raw_str[pos + 1:match.start()]
    else:
        return None


class _StringToken(object):
    def __init__(self, string):
        self.string = string

    def __str__(self):
        return self.string
    def __eq__(self, other):
        return self.string == other.string


def parse(raw_str):
    tokens = _tokenize(raw_str)
    next_token = tokens.next()
    if next_token != '[':
        raise ParseError('[ expected, got %s ' % next_token)
    array = list(_parse_list(tokens))
    for token  in tokens:
        raise ParseError('unexpexted token: %s' % token)
    return array


def _tokenize(raw_str):
    raw_str = raw_str.strip()
    # raw_str
    pos = 0
    while pos < len(raw_str):
        char = raw_str[pos]
        str_match = _match_str(raw_str, pos)
        if str_match is not None:
            unescaped = str_match.decode('string_escape')
            yield _StringToken(unescaped)
            pos += len(str_match) + 2
        elif char in _digit_strings:
            match = _number_matcher.match(raw_str, pos=pos)
            if not match:
                raise ParseError("malformed number")
            number, decimal = match.groups()
            if decimal:
                yield float('%s.%s' % (number, decimal))
                pos += 1 + len(number) + len(decimal)
            else:
                yield int(number)
                pos += len(number)
        else:
            # ignore whitespaces outside strings
            if char not in [' ', '\t', '\n', '\r']:
                yield char
            pos += 1


def _parse_list(tokens):
    while True:
        try:
            token = tokens.next()
        except StopIteration:
            raise ParseError('unexpected end of string')
        if isinstance(token, _StringToken):
            yield str(token)
        elif isinstance(token, int) or isinstance(token, float):
            yield token
        elif token == "[":
            yield list(_parse_list(tokens))
        elif token == "]":
            return
        elif token == ',':
            pass
        else:
            raise ParseError('expected array or string, got: %s' % token)
