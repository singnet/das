-- =============================================================================
-- Test Database Schema for PostgresWrapper Tests
-- =============================================================================
-- 
-- This creates 3 tables:
--   1. organism     - Referenced by feature (FK target)
--   2. cvterm       - Referenced by feature (FK target) - "controlled vocabulary term"
--   3. feature      - Main table with FKs to organism and cvterm
--
-- To use:
--   1. Create database:  createdb postgres_wrapper_test
--   2. Load schema:      psql -d postgres_wrapper_test -f test_schema.sql
--
-- Or with Docker:
--   docker run -d --name pg_test -e POSTGRES_PASSWORD=test -e POSTGRES_DB=postgres_wrapper_test -p 5433:5432 postgres:15
--   docker exec -i pg_test psql -U postgres -d postgres_wrapper_test < test_schema.sql
-- =============================================================================

-- Drop tables if they exist (for re-running)
DROP TABLE IF EXISTS public.feature CASCADE;
DROP TABLE IF EXISTS public.organism CASCADE;
DROP TABLE IF EXISTS public.cvterm CASCADE;

-- =============================================================================
-- Table 1: organism
-- =============================================================================
CREATE TABLE public.organism (
    organism_id SERIAL PRIMARY KEY,
    genus VARCHAR(255) NOT NULL,
    species VARCHAR(255) NOT NULL,
    common_name VARCHAR(255),
    abbreviation VARCHAR(255),
    comment TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

COMMENT ON TABLE public.organism IS 'Stores organism/species information';

-- =============================================================================
-- Table 2: cvterm (Controlled Vocabulary Term)
-- =============================================================================
CREATE TABLE public.cvterm (
    cvterm_id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    definition TEXT,
    is_obsolete BOOLEAN DEFAULT FALSE,
    is_relationshiptype BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

COMMENT ON TABLE public.cvterm IS 'Stores controlled vocabulary terms (types)';

-- =============================================================================
-- Table 3: feature (Main table with Foreign Keys)
-- =============================================================================
CREATE TABLE public.feature (
    feature_id SERIAL PRIMARY KEY,
    organism_id INTEGER NOT NULL REFERENCES public.organism(organism_id),
    type_id INTEGER NOT NULL REFERENCES public.cvterm(cvterm_id),
    name VARCHAR(255),
    uniquename VARCHAR(255) NOT NULL,
    residues TEXT,
    seqlen INTEGER,
    md5checksum VARCHAR(32),
    is_analysis BOOLEAN DEFAULT FALSE,
    is_obsolete BOOLEAN DEFAULT FALSE,
    timeaccessioned TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    timelastmodified TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

COMMENT ON TABLE public.feature IS 'Main feature table with FKs to organism and cvterm';

-- Create indexes for foreign keys
CREATE INDEX idx_feature_organism_id ON public.feature(organism_id);
CREATE INDEX idx_feature_type_id ON public.feature(type_id);
CREATE INDEX idx_feature_uniquename ON public.feature(uniquename);

-- =============================================================================
-- Insert Test Data: Organisms
-- =============================================================================
INSERT INTO public.organism (organism_id, genus, species, common_name, abbreviation, comment) VALUES
    (1, 'Drosophila', 'melanogaster', 'fruit fly', 'Dmel', 'Model organism for genetics'),
    (2, 'Homo', 'sapiens', 'human', 'Hsap', 'Human species'),
    (3, 'Mus', 'musculus', 'mouse', 'Mmus', 'Laboratory mouse'),
    (4, 'Saccharomyces', 'cerevisiae', 'yeast', 'Scer', 'Baker''s yeast'),
    (5, 'Caenorhabditis', 'elegans', 'roundworm', 'Cele', 'Model organism for development');

-- =============================================================================
-- Insert Test Data: CV Terms (Feature Types)
-- =============================================================================
INSERT INTO public.cvterm (cvterm_id, name, definition, is_obsolete, is_relationshiptype) VALUES
    (1, 'gene', 'A region of biological sequence that encodes a gene', FALSE, FALSE),
    (2, 'mRNA', 'Messenger RNA', FALSE, FALSE),
    (3, 'exon', 'A region of the transcript that is included in the mature RNA', FALSE, FALSE),
    (4, 'intron', 'A region of the transcript that is not included in the mature RNA', FALSE, FALSE),
    (5, 'protein', 'A sequence of amino acids', FALSE, FALSE),
    (6, 'chromosome', 'A structure that contains genetic material', FALSE, FALSE),
    (7, 'promoter', 'A regulatory region', FALSE, FALSE),
    (8, 'obsolete_term', 'This term is obsolete', TRUE, FALSE),
    (9, 'part_of', 'Relationship type: part of', FALSE, TRUE),
    (10, 'derives_from', 'Relationship type: derives from', FALSE, TRUE);

-- =============================================================================
-- Insert Test Data: Features
-- =============================================================================

-- Drosophila melanogaster features (organism_id = 1)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, residues, seqlen, is_analysis, is_obsolete) VALUES
    (1, 1, 1, 'white', 'FBgn0003996', 'ATGCGATCGATCG', 13, FALSE, FALSE),
    (2, 1, 1, 'yellow', 'FBgn0004034', 'GCTAGCTAGCTAG', 13, FALSE, FALSE),
    (3, 1, 1, 'vermilion', 'FBgn0003979', NULL, NULL, FALSE, FALSE),
    (4, 1, 2, 'white-RA', 'FBtr0070001', 'ATGCGATCG', 9, FALSE, FALSE),
    (5, 1, 2, 'white-RB', 'FBtr0070002', 'ATGCGA', 6, FALSE, FALSE),
    (6, 1, 3, 'white:1', 'FBexon0001', 'ATG', 3, FALSE, FALSE),
    (7, 1, 3, 'white:2', 'FBexon0002', 'CGA', 3, FALSE, FALSE),
    (8, 1, 5, 'white-PA', 'FBpp0070001', 'MVLSPADKTNVKAAWGKVGAHAGEYGAEALERMFLSFPTTKTYFPHFDLSH', 51, FALSE, FALSE),
    (9, 1, 6, 'chr2L', 'FBchr0000001', NULL, 23513712, FALSE, FALSE),
    (10, 1, 1, 'obsolete_gene', 'FBgn9999999', NULL, NULL, FALSE, TRUE);

-- Human features (organism_id = 2)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, residues, seqlen, is_analysis, is_obsolete) VALUES
    (11, 2, 1, 'BRCA1', 'ENSG00000012048', 'ATGCGATCGATCGATCG', 17, FALSE, FALSE),
    (12, 2, 1, 'TP53', 'ENSG00000141510', 'GCTAGCTAGCTAGCTAG', 17, FALSE, FALSE),
    (13, 2, 2, 'BRCA1-201', 'ENST00000357654', 'ATGCGATCG', 9, FALSE, FALSE),
    (14, 2, 5, 'BRCA1-P01', 'ENSP00000350283', 'MDLSALRVEEVQNVINAMQKILECPI', 26, FALSE, FALSE);

-- Mouse features (organism_id = 3)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, residues, seqlen, is_analysis, is_obsolete) VALUES
    (15, 3, 1, 'Brca1', 'MGI:104537', 'ATGCGATCG', 9, FALSE, FALSE),
    (16, 3, 1, 'Trp53', 'MGI:98834', 'GCTAGCTAG', 9, FALSE, FALSE);

-- Yeast features (organism_id = 4)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, residues, seqlen, is_analysis, is_obsolete) VALUES
    (17, 4, 1, 'GAL4', 'YPL248C', 'ATGCGA', 6, FALSE, FALSE),
    (18, 4, 1, 'ACT1', 'YFL039C', 'GCTAGC', 6, FALSE, FALSE);

-- C. elegans features (organism_id = 5)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, residues, seqlen, is_analysis, is_obsolete) VALUES
    (19, 5, 1, 'unc-54', 'WBGene00006789', 'ATGCGATCGATCG', 13, FALSE, FALSE),
    (20, 5, 1, 'dpy-10', 'WBGene00001072', 'GCTAGCTAGCTAG', 13, FALSE, FALSE);

-- =============================================================================
-- Special Test Cases
-- =============================================================================

-- Feature with NULL name (for testing NULL handling)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename) VALUES
    (21, 1, 1, NULL, 'FBgn_null_name');

-- Feature with empty string name (for testing empty string handling)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename) VALUES
    (22, 1, 1, '', 'FBgn_empty_name');

-- Feature with very long residues (> MAX_VALUE_SIZE = 1000)
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, residues, seqlen) VALUES
    (23, 1, 1, 'long_residues_gene', 'FBgn_long', REPEAT('ATGC', 300), 1200);

-- Feature with special characters in name
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename) VALUES
    (24, 1, 1, 'gene''s "special" <name>', 'FBgn_special_chars');

-- Feature with unicode in name
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename) VALUES
    (25, 1, 1, 'gène_français_日本語', 'FBgn_unicode');

-- Feature with analysis flag
INSERT INTO public.feature (feature_id, organism_id, type_id, name, uniquename, is_analysis) VALUES
    (26, 1, 1, 'computed_gene', 'FBgn_analysis', TRUE);

-- =============================================================================
-- Reset sequences to continue after manual IDs
-- =============================================================================
SELECT setval('public.organism_organism_id_seq', (SELECT MAX(organism_id) FROM public.organism));
SELECT setval('public.cvterm_cvterm_id_seq', (SELECT MAX(cvterm_id) FROM public.cvterm));
SELECT setval('public.feature_feature_id_seq', (SELECT MAX(feature_id) FROM public.feature));

-- =============================================================================
-- Verify data loaded correctly
-- =============================================================================
DO $$
DECLARE
    org_count INTEGER;
    cvterm_count INTEGER;
    feature_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO org_count FROM public.organism;
    SELECT COUNT(*) INTO cvterm_count FROM public.cvterm;
    SELECT COUNT(*) INTO feature_count FROM public.feature;
    
    RAISE NOTICE '===========================================';
    RAISE NOTICE 'Test database loaded successfully!';
    RAISE NOTICE '===========================================';
    RAISE NOTICE 'Organisms: % rows', org_count;
    RAISE NOTICE 'CV Terms:  % rows', cvterm_count;
    RAISE NOTICE 'Features:  % rows', feature_count;
    RAISE NOTICE '===========================================';
END $$;