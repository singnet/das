import argparse
import random
import re
import time
import json
import itertools
from metta_file_generator import NodeLinkGenerator, NodeLinkWriter


PREDICATES = {}

def set_predicate_name(key, name):
    global PREDICATES
    if key in PREDICATES:
        PREDICATES[key].add(name)
    else:
        PREDICATES[key] = set()
        PREDICATES[key].add(name)

def build_transitions(alphabet, dist, rep=0.0, words=None, bonus=0.0):
    if words is None:
        words = []
    
    states = ["".join(p) for p in itertools.product(alphabet, repeat=2)]
    transitions = {}
    
    for state in states:
        probs = {ch: 0.0 for ch in alphabet}
        
        last = state[-1]
        probs[last] += rep
        
        for word in words:
            for i in range(len(word) - 2):
                prefix = word[i:i+2]
                nxt = word[i+2]
                if state == prefix:
                    probs[nxt] += bonus
        
        remaining = 1 - sum(probs.values())

        if remaining < 0:
            raise ValueError(f"Probability overflow in state '{state}', adjust rep/bonus.")
        
        sum_target = sum(dist[ch] for ch in alphabet if probs[ch] == 0)
        for ch in alphabet:
            if probs[ch] == 0:
                probs[ch] = dist[ch] / sum_target * remaining
        
        total = sum(probs.values())
        for ch in probs:
            probs[ch] /= total
        
        transitions[state] = [(ch, probs[ch]) for ch in alphabet]
    
    return transitions

def generate_sentence(length, transitions, initial=None, word_size=None, start_letter=None, end_letter=None):
    states = list(transitions.keys())
    if initial is None:
        initial = random.choice(states)
    
    current = initial
    result = list(current)
    
    for _ in range(length - 2):
        next_chars, probs = zip(*transitions[current])
        nxt = random.choices(next_chars, probs)[0]
        result.append(nxt)
        current = current[1] + nxt  # shift context

    # if word_size and start_letter and end_letter:
    #     # find all the indexes of start_letter and end_letter
    #     start_indexes = [i for i in range(len(result)) if result[i] == start_letter]
    #     end_indexes = [i for i in range(len(result)) if result[i] == end_letter]
    #     # check if there is a valid position to insert start_letter and end_letter
    #     valid_positions = [i for i in start_indexes if i + word_size - 1 < len(result) and result[i + word_size - 1] == end_letter]
    #     if valid_positions:
    #         insert_position = random.choice(valid_positions)
    #         result[insert_position] = start_letter
    #         result[insert_position + word_size - 1] = end_letter

    return "".join(result)

def process_config_file(config_json):
    alphabet_range = config_json.get('alphabet-range')
    alphabet = [chr(97 + i) for i in range(int(alphabet_range.split('-')[0]), int(alphabet_range.split('-')[1]) + 1)]
    default_prob = 1 / len(alphabet)
    default_dist = dict(((l, default_prob) for l in alphabet))
    config_json['default_dist'] = default_dist
    for predicate_group in config_json.get('letter-predicates', []):
        predicate_group_name = str(predicate_group.get('token')).format(letter=predicate_group.get('letter', ''), letter_percent=predicate_group.get('letter_percent', 0))
        prob = (1.0 - predicate_group.get('letter_percent', 0)) / (len(alphabet) - 1)
        dist = dict(((l, prob) for l in alphabet))
        dist[predicate_group['letter']] = predicate_group.get('letter_percent', 0)
        predicate_group['predicate_group_name'] = 'letter-predicates'
        predicate_group['transition'] = build_transitions(alphabet, dist)
        predicate_group['transition_args'] =  {'alphabet': alphabet, 'dist': dist}
        config_json[predicate_group_name] = predicate_group
    for predicate_group in config_json.get('wildcard-predicates', []):
        wildcards = [w.replace('*', '') for w in predicate_group.get('wildcards', [])]
        predicate_group_name = str(predicate_group.get('token')).replace('*', '_'.join(wildcards))
        predicate_group['predicate_group_name'] = 'wildcard-predicates'
        predicate_group['transition'] = build_transitions(alphabet, default_dist, words=wildcards, bonus=0.3)
        predicate_group['transition_args'] =  {'alphabet': alphabet, 'dist': default_dist, 'words': wildcards, 'bonus': 0.3}
        config_json[predicate_group_name] = predicate_group
    for predicate_group in config_json.get('start-end-predicates', []):
        predicate_group_name = str(predicate_group.get('token')).format(start_letter=predicate_group.get('start_letter', ''), end_letter=predicate_group.get('end_letter', ''))
        predicate_group['predicate_group_name'] = 'start-end-predicates'
        predicate_group['transition_args'] =  {'alphabet': alphabet, 'dist': default_dist}
        config_json[predicate_group_name] = predicate_group
    for predicate_group in config_json.get('low-diversity-predicates', []):
        predicate_group_name = str(predicate_group.get('token')).format(letters='_'.join(predicate_group.get('letters', [])))
        predicate_group['predicate_group_name'] = 'low-diversity-predicates'
        predicate_group['transition'] = build_transitions(alphabet, default_dist, rep=0.5)
        predicate_group['transition_args'] =  {'alphabet': alphabet, 'dist': default_dist, 'rep':0.5}
        config_json[predicate_group_name] = predicate_group
    for p in config_json.get('bias', {}).get('predicates', []):
        if p in config_json:
            continue
        key = config_json.get('word-predicate', {}).get('token').replace('*', '')
        if p.startswith(key):
            words = p.replace(key, '').split('_')
            config_json[p] = config_json.get('word-predicate', {}).copy()
            config_json[p]['words_to_find'] = words
            config_json[p]['predicate_group_name'] = 'word-predicate'
            config_json[p]['transition'] = build_transitions(alphabet, default_dist, words=words, bonus=0.3)
            config_json[p]['transition_args'] =  {'alphabet': alphabet, 'dist': default_dist, 'words': words, 'bonus': 0.3}


def parse_args():
    parser = argparse.ArgumentParser(description="Process some sentences.")
    parser.add_argument("file", type=str, help="The file to process")
    parser.add_argument("--config-file", type=str, help="Path to the configuration file")
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
    set_predicate_name('sorting_predicate', predicate_name)
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
    set_predicate_name('letter_predicate', predicate_name)
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
    set_predicate_name('wildcard_predicate', predicate_name)
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
    set_predicate_name('start_end_predicate', predicate_name)
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
    set_predicate_name('low_diversity_predicate', predicate_name)
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
                
def word_predicate(sentences, token=None, word_count=1, alphabet_range="0-4", percentage=0.1, words_to_find=[], **kwargs):
    if kwargs:
        token = kwargs.get('word_predicate_token', token)
        word_count = kwargs.get('word_predicate_word_count', word_count)
        alphabet_range = kwargs.get('word_predicate_alphabet_range', alphabet_range)
        percentage = kwargs.get('word_predicate_percentage', percentage)
        word_length = kwargs.get('word_length', 1)
    show_log = kwargs.get('show_log', True)
    if show_log:
        print(f"### Running word predicate with token: {token}, word_count: {word_count}, alphabet_range: {alphabet_range}, percentage: {percentage}")
    words = words_to_find or generate_words(word_length, alphabet_range)
    word_count = len(words_to_find) if words_to_find else word_count
    predicates = []
    for sentence in sentences:
        split_sentence = sentence[:-1].replace('"', '').split(' ')[1:]
        if word_count == 1:
            for word1 in words:
                if word1 in split_sentence:
                    if random.random() < percentage:
                        predicate_n = word1
                        predicate_token = token.replace('*', predicate_n)
                        set_predicate_name('word_predicate', predicate_token)
                        predicates.append(f'(EVALUATION (PREDICATE "{predicate_token}") (CONCEPT {sentence}))')
        elif word_count == 2:
            for word1 in words:
                for word2 in words:
                    if word1 != word2 and word1 in split_sentence and word2 in split_sentence:
                        if random.random() < percentage:
                            predicate_n = '_'.join([word1, word2])
                            predicate_token = token.replace('*', predicate_n)
                            set_predicate_name('word_predicate', predicate_token)
                            predicates.append(f'(EVALUATION (PREDICATE "{predicate_token}") (CONCEPT {sentence}))')
        else:
            print(f"### ERROR: invalid word_count: {word_count}")

    return predicates
        
def word_list_to_sentence(word_list):
    sentence = ' '.join(word_list)
    return f'(Sentence "{ sentence }")'

def merge_and_build_transition_args(transition_args):
    out = {}
    for transition in transition_args:
        dist = transition.get('dist')
        if 'dist' in out:
            if dist is not None and any((v == 1.0 / len(dist) for v in dist.values())):
                del transition['dist']
        out.update(transition)
    out['rep'] = max((transition.get('rep', 0) for transition in transition_args if transition.get('rep', 0) is not None), default=0)
    out['bonus'] = max((transition.get('bonus', 0) for transition in transition_args if transition.get('bonus', 0) is not None), default=0)
    out['words'] = list(set(itertools.chain.from_iterable((transition.get('words', []) for transition in transition_args if transition.get('words', []) is not None))))
    return build_transitions(**out)

def create_predicates(args, predicate_funcs, sentence_count, predicate_names):
    sentences = []
    predicates = []
    generator = NodeLinkGenerator(
        int(1),
        int(args.get('word_count', 3)),
        int(args.get('word_length', 3)),
        args.get('alphabet_range', '0-4'),
        debug=True,
    )
    writer = NodeLinkWriter(filename=args.get('bias', {}).get('filename', 'test_biased_predicates.metta'))
    args_cp = args.copy()
    count = 0
    sentence_counting = 0

    sentence_length = args.get('word_count', 3) * args.get('word_length', 3)
    transition = merge_and_build_transition_args([args.get(n).get('transition_args') for n in predicate_names])
    start_time = time.time()
    while count < sentence_count:
        # Commenting random generator
        # sentence = generator.generate_sentence()
        # Log the percentage of sentences generated once every 5 seconds
        if time.time() - start_time > 5:
            start_time = time.time()
            if sentence_counting > 0:
                print(f"### Generated {(count / sentence_count * 100):.2f}% sentences so far to create {count}/{sentence_count} biased sentences.")
            else:
                print(f"### Generated 0.00% sentences so far to create {count}/{sentence_count} biased sentences.")

        sentence = generate_sentence(sentence_length, transition)
        sentence = [sentence[i:i + args.get('word_length', 3)] for i in range(0, len(sentence), args.get('word_length', 3))]
        sentence_counting += 1
        predicates_list = []
        metta_sentence = word_list_to_sentence(sentence)
        for i, predicate_func in enumerate(predicate_funcs):
            pred_args = args.get(predicate_names[i]).copy()
            pred_args['show_log'] = False
            pred_args['percentage'] = 1
            preds = list(predicate_func([metta_sentence], **pred_args))
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

def generate_biased_predicates(args):
    print(f"### Generating biased predicates based on the provided arguments, predicates: {args.get('bias', {}).get('predicates', [])}, strength: {args.get('bias', {}).get('strength', 0)}")
    biased_predicates = []
    predicate_funcs = {
        'letter-predicates': letter_predicate,
        'wildcard-predicates': wildcard_predicate,
        'start-end-predicates': start_end_predicate,
        'low-diversity-predicates': low_diversity_predicate,
        'word-predicate': word_predicate
    }
    bias_count = int(args.get('sentence-node-count') * args.get('bias', {}).get('strength', 0))


    if bias_count <= 0:
        print("### Bias strength is too low, no biased predicates will be generated.")
        return biased_predicates
    if args.get('bias', {}).get('operator', '') == 'and':
        biased_predicates.extend(create_predicates(args, [predicate_funcs[args.get(p).get('predicate_group_name')] for p in args.get('bias', {}).get('predicates', [])], bias_count, [p for p in args.get('bias', {}).get('predicates', [])]))
    elif args.get('bias', {}).get('operator', '') == 'or':
        for i, predicate in enumerate(args.get('bias', {}).get('predicates', [])):
            biased_predicates.extend(create_predicates(args, [predicate_funcs[args.get(predicate).get('predicate_group_name')]], bias_count / (i + 1), [predicate]))
    write_predicates(args.get('bias').get('filename'), biased_predicates, write_words_and_sentences=False, append=True)
    return biased_predicates

def main():
    args = parse_args()
    config_json = json.load(open(args.config_file))
    process_config_file(config_json)
    # initialize seed
    random.seed(config_json.get('seed', 42))
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
    letter_predicates = []
    for predicate in config_json.get('letter-predicates', []):
        lp = letter_predicate(sentences, 
                              predicate.get('token', None), 
                              predicate.get('letter', None), 
                              predicate.get('letter_percent', None), 
                              predicate.get('percentage', None),)
        letter_predicates.extend(lp)
    letter_predicates_duration = time.time() - letter_predicates_time
    predicates.extend(letter_predicates)
    # Wildcard predicates
    wildcard_predicates_time = time.time()
    wildcard_predicates = []
    for predicate in config_json.get('wildcard-predicates', []):
        wp = wildcard_predicate(sentences, 
                                predicate.get('token', None), 
                                predicate.get('wildcards', None), 
                                predicate.get('percentage', None))
        wildcard_predicates.extend(wp)
    wildcard_predicates_duration = time.time() - wildcard_predicates_time
    predicates.extend(wildcard_predicates)
    # Start-end predicates
    start_end_predicates = []
    start_end_predicates_time = time.time()
    for predicate in config_json.get('start-end-predicates', []):
        sep = start_end_predicate(sentences, 
                                 predicate.get('token', None), 
                                 predicate.get('start_letter', None), 
                                 predicate.get('end_letter', None), 
                                 predicate.get('percentage', None))
        start_end_predicates.extend(sep)
    start_end_predicates_duration = time.time() - start_end_predicates_time
    predicates.extend(start_end_predicates)
    # Low-diversity predicates
    low_diversity_predicates = []
    low_diversity_predicates_time = time.time()
    for predicate in config_json.get('low-diversity-predicates', []):
        ldp = low_diversity_predicate(sentences, 
                                     predicate.get('token', None), 
                                     predicate.get('letters', None), 
                                     predicate.get('percentage', None))
        low_diversity_predicates.extend(ldp)
    low_diversity_predicates_duration = time.time() - low_diversity_predicates_time
    predicates.extend(low_diversity_predicates)

    # Word predicates
    word_predicates_time = time.time()
    predicate = config_json.get('word-predicate', {})
    if predicate:
        word_predicates = word_predicate(sentences, 
                            predicate.get('token', None), 
                            predicate.get('word-count', None), 
                            predicate.get('alphabet-range', None), 
                            predicate.get('percentage', None),
                            word_length=config_json.get('word-length'))

        word_predicates_duration = time.time() - word_predicates_time
        predicates.extend(word_predicates)
    write_predicates(config_json.get('metta-filename'), predicates, 
                     write_words_and_sentences=False, append=True)
    biased_predicates_time = time.time()
    biased_predicates = []
    if config_json.get('bias', {}).get('strength', 0) > 0:
        biased_predicates = generate_biased_predicates(config_json)
        predicates.extend(biased_predicates)
    biased_predicates_duration = time.time() - biased_predicates_time
    
    total_time = time.time() - total_time


    # Report
    print("######################### Summary #########################")
    print(f"Generated {len(predicates)} predicates based on the sentences in {args.file}.")
    print(f"Letter predicates: {len(letter_predicates)}, predicates number: {len(PREDICATES.get('letter_predicate', set()))}")
    print(f"Wildcard predicates: {len(wildcard_predicates)}, predicates number: {len(PREDICATES.get('wildcard_predicate', set()))}")
    print(f"Start-end predicates: {len(start_end_predicates)}, predicates number: {len(PREDICATES.get('start_end_predicate', set()))}")
    print(f"Low-diversity predicates: {len(low_diversity_predicates)}, predicates number: {len(PREDICATES.get('low_diversity_predicate', set()))}")
    print(f"Word predicates: {len(word_predicates)}, predicates number: {len(PREDICATES.get('word_predicate', set()))}")
    print(f"Biased predicates: {len(biased_predicates)}")
    print("######################### Duration #########################")
    print(f"Read sentences duration: {read_sentences_duration:.2f} seconds")
    print(f"Letter predicates duration: {letter_predicates_duration:.2f} seconds")
    print(f"Wildcard predicates duration: {wildcard_predicates_duration:.2f} seconds")
    print(f"Start-end predicates duration: {start_end_predicates_duration:.2f} seconds")
    print(f"Low-diversity predicates duration: {low_diversity_predicates_duration:.2f} seconds")
    print(f"Biased predicates duration: {biased_predicates_duration:.2f} seconds")
    print(f"Total time: {total_time:.2f} seconds")

main()
