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
