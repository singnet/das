import argparse
import random
import re
import time
import json
import itertools
from metta_file_generator import NodeLinkGenerator, NodeLinkWriter

def parse_args():
    parser = argparse.ArgumentParser(description="Process some sentences.")
    parser.add_argument("file", type=str, help="The file to process")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility")
    parser.add_argument("--config-file", type=str, default=None, help="Path to the configuration file")
    parser.add_argument("--letter-predicate-token", type=str, default='feature_a', help="Token for letter predicate")
    parser.add_argument("--letter-predicate-letter", type=str, default='a', help="Letter for letter predicate")
    parser.add_argument("--letter-predicate-percent", type=float, default=0.6, help="Percentage of letters for letter predicate")
    parser.add_argument("--letter-predicate-letter-percent", type=float, default=0.6, help="Percentage of letters for letter predicate")
    parser.add_argument("--letter-predicate-percentage", type=float, default=0.8, help="Randomness for letter predicate")
    parser.add_argument("--wildcard-predicate-token", type=str, default='feature_b', help="Token for wildcard predicate")
    parser.add_argument("--wildcard-predicate-wildcards", type=str, nargs='+', default=['*ac', 'cb*'], help="Wildcards for wildcard predicate")
    parser.add_argument("--wildcard-predicate-percentage", type=float, default=0.8, help="Randomness for wildcard predicate")
    parser.add_argument("--start-end-predicate-token", type=str, default='feature_c', help="Token for start-end predicate")
    parser.add_argument("--start-end-predicate-start-letter", type=str, default='c', help="Start letter for start-end predicate")
    parser.add_argument("--start-end-predicate-end-letter", type=str, default='d', help="End letter for start-end predicate")
    parser.add_argument("--start-end-predicate-percentage", type=float, default=0.8, help="Randomness for start-end predicate")
    parser.add_argument("--low-diversity-predicate-token", type=str, default='feature_d', help="Token for low-diversity predicate")
    parser.add_argument("--low-diversity-predicate-letters", type=str, nargs='+', default=['a', 'b', 'c', 'd', 'e'], help="Letters for low-diversity predicate")
    parser.add_argument("--low-diversity-predicate-percentage", type=float, default=0.8, help="Randomness for low-diversity predicate")
    parser.add_argument("--word-predicate-token", type=str, default='word_*', help="Token for word predicate, use * for numbering")
    parser.add_argument("--word-predicate-percentage", type=float, default=0.8, help="Randomness for word predicate")
    parser.add_argument("--word-predicate-alphabet-range", type=str, default="0-4", help="Alphabet range for word predicate, eg: 0-4")
    parser.add_argument("--word-predicate-word-count", type=int, default=3, help="Number of words to match for word predicate")
    parser.add_argument("--bias-predicates", type=str, nargs='+', default=['letter-predicate', 'low-diversity-predicate'], help="Predicates to bias")
    parser.add_argument("--bias-strength", type=float, default=0.5, help="Strength of the bias")
    parser.add_argument("--bias-filename", type=str, default='bias.metta', help="Filename for biased predicates")
    parser.add_argument("--metta-filename", type=str, default='ooo.metta', help="Output MeTTa filename")
    parser.add_argument("--sentence-node-count", type=int, default=3, help="Number of Sentence nodes to be generated")
    parser.add_argument("--word-count", type=int, default=3, help="How many words make up the node")
    parser.add_argument("--word-length", type=int, default=3, help="Number of characters in the word")
    parser.add_argument("--alphabet-range", type=str, default="0-3", help="Alphabet range, eg: 2-5")
    parser.add_argument("--bias-operator", type=str, default='and', choices=['and', 'or'], help="Operator for combining biased predicates")
    return parser.parse_args()

def read_sentences(file_path):
    with open(file_path, 'r') as file:
        sentences = file.readlines()
    return [sentence.strip() for sentence in sentences if sentence.strip() and sentence.startswith('(Sentence')]

def parse_config_file(config_file, args):
    config = {}
    with open(config_file, 'r') as file:
        config = json.load(file)
        config_copy = {}
        for k,v in config.items():
            if isinstance(v, dict):
                for k2, v2 in v.items():
                    config_copy[f"{k}-{k2}"] = v2
            else:
                config_copy[k] = v
        config = config_copy
    for key, value in config.items():
        key = key.replace('-', '_') 
        if not hasattr(args, key):
            print(f"Warning: Config key '{key}' does not match any argument in the script.")
        setattr(args, key, value)
    return args

def sentence2list(sentence):
    return sentence.split('"')[1].split(" ")

def list2sentence(list_sentence):
    return f'(Sentence "{" ".join(list_sentence)}")'

def sorting_predicate(sentences, token=None, percentage=1, **kwargs):
    if kwargs:
        token = kwargs.get('sorting_predicate_token', token)
        percentage = kwargs.get('sorting_predicate_percentage', percentage)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running sorting predicate with token: {token}, percentage: {percentage}, sentences: {len(sentences)}")
    predicate_name = str(token)
    predicates = []
    for sentence in sentences:
        parsed_sentence = sentence.split('"')[1].split(" ")
        if sorted(parsed_sentence) == parsed_sentence and random.random() < percentage:
            predicates.append(f'(EVALUATION (PREDICATE "{predicate_name}") (CONCEPT {sentence}))')
    return predicates

def sorting_bias(sentences, count_rate, probability):
    answer = []
    count = 0
    target_count = len(sentences) * count_rate
    for sentence in sentences:
        sentence_list = sentence2list(sentence)
        if count < target_count:
            count += 1
            sentence_list[0:3] = ["ddd", "ccc", "bbb"]
            if random.random() < probability:
                sentence_list.sort()
            sentence = list2sentence(sentence_list)
        answer.append(sentence)
    return answer

def letter_predicate(sentences, token=None, letter=None, letter_percent=0.6, percentage=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('letter_predicate_token', token)
        letter = kwargs.get('letter_predicate_letter', letter)
        letter_percent = kwargs.get('letter_predicate_letter_percent', letter_percent)
        percentage = kwargs.get('letter_predicate_percentage', percentage)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running letter predicate with token: {token}, letter: {letter}, letter_percent: {letter_percent}, percentage: {percentage}, sentences: {len(sentences)}")
    predicate_name = str(token).format(letter=letter, letter_percent=letter_percent)
    predicates = []
    for sentence in sentences:
        parsed_sentence = ''.join(sentence.replace(' ', '').split('"')[1:-1])
        count_letter = parsed_sentence.count(letter)
        if count_letter > 0 and count_letter / len(parsed_sentence) >= letter_percent:
            if random.random() < percentage:
                predicates.append(f'(EVALUATION (PREDICATE "{predicate_name}") (CONCEPT {sentence}))')
    return predicates

def wildcard_predicate(sentences, token=None, wildcards=None, percentage=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('wildcard_predicate_token', token)
        wildcards = kwargs.get('wildcard_predicate_wildcards', wildcards)
        percentage = kwargs.get('wildcard_predicate_percentage', percentage)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running wildcard predicate with token: {token}, wildcards: {wildcards}, percentage: {percentage}")
    predicates = []
    predicate_name = str(token).replace('*', '_'.join([w.replace('*', '') for w in wildcards]))
    for sentence in sentences:
        split_sentence = sentence[:-1].replace('"', '').split(' ')[1:]
        for s in split_sentence:
            for wildcard in wildcards:
                if re.match(wildcard.replace('*', '.'), s):
                    if random.random() < percentage:
                        predicates.append(f'(EVALUATION (PREDICATE "{predicate_name}") (CONCEPT {sentence}))')
                    break
    return predicates

def start_end_predicate(sentences, token=None, start_letter=None, end_letter=None, percentage=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('start_end_predicate_token', token)
        start_letter = kwargs.get('start_end_predicate_start_letter', start_letter)
        end_letter = kwargs.get('start_end_predicate_end_letter', end_letter)
        percentage = kwargs.get('start_end_predicate_percentage', percentage)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running start-end predicate with token: {token}, start_letter: {start_letter}, end_letter: {end_letter}, percentage: {percentage}")
    predicates = []
    predicate_name = str(token).format(start_letter=start_letter, end_letter=end_letter)
    for sentence in sentences:
        split_sentence = sentence[:-1].replace('"', '').split(' ')[1:]
        for s in split_sentence:
            if s.startswith(start_letter) and s.endswith(end_letter):
                if random.random() < percentage:
                    predicates.append(f'(EVALUATION (PREDICATE "{predicate_name}") (CONCEPT {sentence}))')
                    break
    return predicates

def low_diversity_predicate(sentences, token=None, letters=None, percentage=0.8, **kwargs):
    if kwargs:
        token = kwargs.get('low_diversity_predicate_token', token)
        letters = kwargs.get('low_diversity_predicate_letters', letters)
        percentage = kwargs.get('low_diversity_predicate_percentage', percentage)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running low-diversity predicate with token: {token}, letters: {letters}, percentage: {percentage}")
    predicates = []
    predicate_name = str(token).format(letters='_'.join(letters))
    for sentence in sentences:
        parsed_sentence = ''.join(sentence.replace(' ', '').split('"')[1:-1])
        letter_count = 0
        for letter in letters:
            if parsed_sentence.count(letter) > 0:
                letter_count += 1
        if letter_count <= len(letters) / 2 and random.random() < percentage:
            predicates.append(f'(EVALUATION (PREDICATE "{predicate_name}") (CONCEPT {sentence}))')

    return predicates

WORDS = []

def generate_words(word_length, alphabet_range):
    global WORDS
    if WORDS:
        return WORDS
    alphabet = [chr(97 + i) for i in range(int(alphabet_range.split('-')[0]), int(alphabet_range.split('-')[1]) + 1)]
    WORDS = [''.join(p) for p in list(itertools.product(*[''.join(alphabet)]*word_length))]
    return WORDS
                

def word_predicate(sentences, token=None, word_count=1, alphabet_range="0-4", percentage=0.1, **kwargs):
    if kwargs:
        token = kwargs.get('word_predicate_token', token)
        word_count = kwargs.get('word_predicate_word_count', word_count)
        alphabet_range = kwargs.get('word_predicate_alphabet_range', alphabet_range)
        percentage = kwargs.get('word_predicate_percentage', percentage)
        word_length = kwargs.get('word_length', 1)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running word predicate with token: {token}, word_count: {word_count}, alphabet_range: {alphabet_range}, percentage: {percentage}")
    words = generate_words(word_length, alphabet_range)
    predicates = []
    for sentence in sentences:
        split_sentence = sentence[:-1].replace('"', '').split(' ')[1:]
        if word_count == 1:
            for word1 in words:
                if word1 in split_sentence and word2 in split_sentence:
                    if random.random() < percentage:
                        predicate_n = word1
                        predicate_token = token.replace('*', predicate_n)
                        predicates.append(f'(EVALUATION (PREDICATE "{predicate_token}") (CONCEPT {sentence}))')
        elif word_count == 2:
            for word1 in words:
                for word2 in words:
                    if word1 != word2 and word1 in split_sentence and word2 in split_sentence:
                        if random.random() < percentage:
                            predicate_n = '_'.join([word1, word2])
                            predicate_token = token.replace('*', predicate_n)
                            predicates.append(f'(EVALUATION (PREDICATE "{predicate_token}") (CONCEPT {sentence}))')
        else:
            print(f"### ERROR: invalid word_count: {word_count}")

    return predicates
        

def word_list_to_sentence(word_list):
    sentence = ' '.join(word_list)
    return f'(Sentence "{ sentence }")'

def create_predicates(args, predicate_funcs, sentence_count):
    sentences = []
    predicates = []
    generator = NodeLinkGenerator(
        int(1),
        int(args.word_count),
        int(args.word_length),
        args.alphabet_range,
        debug=True,
    )
    writer = NodeLinkWriter(filename=args.bias_filename)
    args_cp = vars(args)
    for k in args_cp.keys():
        if 'percentage' in k:
            args_cp[k] = 1
    args_cp['show_log'] = False
    count = 0
    sentence_counting = 0
    while count < sentence_count:
        sentence = generator.generate_sentence()
        sentence_counting += 1
        predicates_list = []
        metta_sentence = word_list_to_sentence(sentence)
        for predicate_func in predicate_funcs:
            preds = list(predicate_func([metta_sentence], **vars(args)))
            if not preds:
                predicates_list = []
                break
            predicates_list.extend(preds)
        if len(predicates_list) == len(predicate_funcs):
            count += 1
            generator.write_atoms(sentence, writer)
            for p in predicates_list:
                predicates.append(p)
    print(f"### Generated {sentence_counting} sentences to create {count} biased sentences.")
    writer.commit_changes()

    return predicates

def write_predicates(filename, biased_predicates, write_words_and_sentences=True, append=False):
    sbuffer = set()
    wbuffer = set()
    with open(filename, 'w' if not append else 'a') as f:
        if write_words_and_sentences:
            for predicate in biased_predicates:
                sentence  = '(' + predicate.split('(')[-1][:-2]
                words = sentence[:-1].replace('"', '').split(' ')[1:]
                for word in words:
                    wbuffer.add(f'(: "{word}" Word)\n')
                sbuffer.add(sentence + '\n')
            for w in wbuffer:
                f.write(w)
            for b in sbuffer:
                f.write(b)
        for predicate in biased_predicates:
            f.write(predicate + '\n')

def generate_biased_predicates(args, predicate_lengths):
    print(f"### Generating biased predicates based on the provided arguments, predicates: {args.bias_predicates}, strength: {args.bias_strength}")
    biased_predicates = []
    predicate_funcs = {
        'letter-predicate': letter_predicate,
        'wildcard-predicate': wildcard_predicate,
        'start-end-predicate': start_end_predicate,
        'low-diversity-predicate': low_diversity_predicate,
        'word-predicate': word_predicate
    }
    bias_count = int(args.sentence_node_count * args.bias_strength)
    if args.bias_operator == 'and':
        biased_predicates.extend(create_predicates(args, [predicate_funcs[p] for p in args.bias_predicates], bias_count))
    elif args.bias_operator == 'or':
        for predicate in args.bias_predicates:
            if predicate in predicate_funcs:
                biased_predicates.extend(create_predicates(args, [predicate_funcs[predicate]], bias_count))
            else:
                print(f"Warning: Predicate '{predicate}' not found in predicate functions. Skipping bias generation for this predicate.")
    write_predicates(args.bias_filename, biased_predicates, write_words_and_sentences=False, append=True)
    return biased_predicates

def main():
    args = parse_args()
    if args.config_file:
        args = parse_config_file(args.config_file, args)
    # initialize seed
    random.seed(args.seed)
    total_time = time.time()
    predicates = []
    # Read sentences from the file
    print(f"Reading sentences from {args.file}...")
    read_sentences_time = time.time()
    non_biased_sentences = read_sentences(args.file)
    read_sentences_duration = time.time() - read_sentences_time
    print(f"Read {len(non_biased_sentences)} sentences from the file.")
    sentences = sorting_bias(non_biased_sentences, 0.1, 0.9)

    
    # NOTE TO REVIEWER:
    # We're not supposed to create these links. This code is here just to help debuging
    #
    # Create predicates based on the order of words in the sentences
    #sorting_predicate_time = time.time()
    #sorting_predicates = sorting_predicate(sentences, 
    #                                       args.sorting_predicate_token, 
    #                                       args.sorting_predicate_percentage)
    #sorting_predicate_duration = time.time() - sorting_predicate_time
    #predicates.extend(sorting_predicates)

    # Create predicates based on the sentences
    letter_predicates_time = time.time()
    letter_predicates = letter_predicate(sentences, 
                                         args.letter_predicate_token, 
                                         args.letter_predicate_letter, 
                                         args.letter_predicate_letter_percent, 
                                         args.letter_predicate_percentage)
    letter_predicates_duration = time.time() - letter_predicates_time
    predicates.extend(letter_predicates)
    # Wildcard predicates
    wildcard_predicates_time = time.time()
    wildcard_predicates = wildcard_predicate(sentences, 
                                             args.wildcard_predicate_token, 
                                             args.wildcard_predicate_wildcards, 
                                             args.wildcard_predicate_percentage)
    wildcard_predicates_duration = time.time() - wildcard_predicates_time
    predicates.extend(wildcard_predicates)
    # Start-end predicates
    start_end_predicates_time = time.time()
    start_end_predicates = start_end_predicate(sentences, 
                                                args.start_end_predicate_token, 
                                                args.start_end_predicate_start_letter, 
                                                args.start_end_predicate_end_letter, 
                                                args.start_end_predicate_percentage)
    start_end_predicates_duration = time.time() - start_end_predicates_time
    predicates.extend(start_end_predicates)
    # Low-diversity predicates
    low_diversity_predicates_time = time.time()
    low_diversity_predicates = low_diversity_predicate(sentences, 
                                                       args.low_diversity_predicate_token, 
                                                       args.low_diversity_predicate_letters, 
                                                       args.low_diversity_predicate_percentage)
    low_diversity_predicates_duration = time.time() - low_diversity_predicates_time
    predicates.extend(low_diversity_predicates)


    word_predicates_time = time.time()
    word_predicates = word_predicate(sentences,
                                     args.word_predicate_token,
                                     args.word_predicate_word_count,
                                     args.word_predicate_alphabet_range,
                                     args.word_predicate_percentage,
                                     word_length=args.word_length)
    word_predicates_duration = time.time() - word_predicates_time
    predicates.extend(word_predicates)
    write_predicates(args.metta_filename, predicates, 
                     write_words_and_sentences=False, append=True)
    biased_predicates_time = time.time()
    if args.bias_strength > 0:
        predicate_lengths = {
            'letter-predicate': len(letter_predicates),
            'wildcard-predicate': len(wildcard_predicates),
            'start-end-predicate': len(start_end_predicates),
            'low-diversity-predicate': len(low_diversity_predicates),
            'word-predicate': len(word_predicates)
        }
        biased_predicates = generate_biased_predicates(args, predicate_lengths)
        # predicates.extend(biased_predicates)
    biased_predicates_duration = time.time() - biased_predicates_time
    
    total_time = time.time() - total_time


    # Report
    print("######################### Summary #########################")
    print(f"Generated {len(predicates)} predicates based on the sentences in {args.file}.")
    print(f"Letter predicates: {len(letter_predicates)}")
    print(f"Wildcard predicates: {len(wildcard_predicates)}")
    print(f"Start-end predicates: {len(start_end_predicates)}")
    print(f"Low-diversity predicates: {len(low_diversity_predicates)}")
    print(f"Word predicates: {len(word_predicates)}")
    print(f"Biased predicates: {len(biased_predicates) if args.bias_strength > 0 else 0}")
    print("######################### Duration #########################")
    print(f"Read sentences duration: {read_sentences_duration:.2f} seconds")
    print(f"Letter predicates duration: {letter_predicates_duration:.2f} seconds")
    print(f"Wildcard predicates duration: {wildcard_predicates_duration:.2f} seconds")
    print(f"Start-end predicates duration: {start_end_predicates_duration:.2f} seconds")
    print(f"Low-diversity predicates duration: {low_diversity_predicates_duration:.2f} seconds")
    print(f"Biased predicates duration: {biased_predicates_duration:.2f} seconds")
    print(f"Total time: {total_time:.2f} seconds")

main()
