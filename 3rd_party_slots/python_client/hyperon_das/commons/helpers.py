from hyperon_das.logger import log


def tokenize_preserve_quotes(s: str) -> list[str]:
    """
    Split the input string into tokens by whitespace, keeping quoted substrings (including the quotes) as single tokens.

    Args:
        s (str): The input string to tokenize.

    Returns:
        list[str]: A list of tokens where text outside quotes is split on spaces, and quoted segments remain intact.
    """
    tokens = []
    current = []
    inside_quotes = False

    i = 0
    while i < len(s):
        c = s[i]

        if c == '"':
            if inside_quotes:
                current.append(c)
                tokens.append(''.join(current))
                current = []
                inside_quotes = False
            else:
                if current:
                    tokens.append(''.join(current))
                    current = []
                current.append(c)
                inside_quotes = True
        elif c == ' ' and not inside_quotes:
            if current:
                tokens.append(''.join(current))
                current = []
        else:
            current.append(c)

        i += 1

    if current:
        tokens.append(''.join(current))

    return tokens


def str_2_bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', '1'):
        return True
    elif v.lower() in ('no', 'false', '0'):
        return False
    else:
        raise ValueError('Boolean value expected.')


def error(msg: str, throw_flag: bool = True):
    log.error(msg)
    if throw_flag:
        raise RuntimeError(msg)
