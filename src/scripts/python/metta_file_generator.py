import argparse
from pathlib import Path
from random import randint


class NodeLinkGenerator:
    def __init__(
        self,
        sentence_node_count: str,
        word_count: str,
        word_length: str,
        alphabet_range: str,
        debug: bool,
    ) -> None:
        self.sentence_node_count: int = int(sentence_node_count)
        self.word_node_count: int = 0
        self.word_count: int = int(word_count)
        self.word_length: int = int(word_length)
        self.alphabet_range: list[int] = [int(i) for i in alphabet_range.split('-')]
        self.link_word_count: int = 0
        self.link_letter_count: int = 0

    def generate_metta_file(self, writer: 'NodeLinkWriter') -> None:
        self._generate_sentence_structure(writer)
        writer.close()

    def print_results(self):
        print('Sentence Nodes: ' + str(self.sentence_node_count))
        print('Word Count:', self.word_count)
        print('Word Length:', self.word_length)
        print('Alphabet Range: ' + ' to '.join((chr(97 + k) for k in self.alphabet_range)))
        print('Word Nodes: ' + str(self.word_node_count))

    def _generate_sentence_structure(self, writer: 'NodeLinkWriter') -> None:
        for _ in range(self.sentence_node_count):
            word_list = [self._create_word() for _ in range(self.word_count)]
            sentence_name = ' '.join(word_list)
            sentence_node = {'type': 'Sentence', 'name': sentence_name}
            writer.add_node(sentence_node)
            sentence_link = {'type': 'Sentence', 'targets': [sentence_node]}
            writer.add_link(sentence_link)

            for word in word_list:
                words_node = {'type': 'Word', 'name': word}
                writer.add_node(words_node)
                self.word_node_count += 1

                words_link = {'type': 'Word', 'targets': [words_node]}
                writer.add_link(words_link)

                contains_link = {'type': 'Contains', 'targets': [sentence_link, words_link]}
                writer.add_link(contains_link)

    def _create_word(self) -> str:
        return ''.join([chr(97 + randint(*self.alphabet_range)) for _ in range(self.word_length)])


class NodeLinkWriter:

    base_path = Path(__file__).resolve().parent

    def __init__(self, filename, batch_size=1000) -> None:
        self.filename = self.base_path / filename
        self.file = open(self.filename, '+a', encoding='utf8')
        self.batch_size = batch_size
        self.node_count = 0
        self.link_count = 0
        self.types = set()

    def add_node(self, node: dict) -> None:
        self._add_type(node.get('type'))
        self.file.write(f"(: \"{node['name']}\" {node['type']})\n")
        self.node_count += 1
        self.flush()

    def add_link(self, link: dict) -> None:
        self._add_type(link.get('type'))
        self.file.write(self._build_expression(link) + '\n')
        self.link_count += 1
        self.flush()

    def close(self) -> None:
        if self.file:
            self.file.close()
            self.file = None

    def _add_type(self, type_name: str) -> None:
        if type_name not in self.types:
            self.types.add(type_name)
            self.file.write(f'(: {type_name} Type)\n')

    def _build_expression(self, link) -> str:
        expression = f"({link['type']}"
        for target in link['targets']:
            if 'name' in target:
                expression += f' "{target["name"]}"'
            else:
                expression += " " + self._build_expression(target)
        if 'tv' in link:
            expression += " (tv " + str(link['tv'][0]) + str(link['tv'][1]) + ")"
        expression += ")"
        return expression

    def flush(self):
        total = self.node_count + self.link_count
        if total % self.batch_size == 0:
            self.file.flush()


def main():
    parser = argparse.ArgumentParser(description='Create MeTTa file.')

    parser.add_argument("--filename", default='output.metta', help="Filename")
    parser.add_argument("--sentence-count", default=3, help="Number of Sentence nodes to be generated, eg: 100")
    parser.add_argument("--word-count", default=3, help="How many words make up the node, eg: 10")
    parser.add_argument("--word-length", default=3, help="Number of characters in the world, eg: 5")
    parser.add_argument("--alphabet-range", default="0-3", help="Alphabet range, eg: 0-5 means a to f")

    args = parser.parse_args()

    generator = NodeLinkGenerator(
        int(args.sentence_count),
        int(args.word_count),
        int(args.word_length),
        args.alphabet_range,
        debug=True,
    )
    generator.generate_metta_file(writer=NodeLinkWriter(args.filename))
    generator.print_results()


if __name__ == '__main__':
    main()
