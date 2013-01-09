
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <cmath>
#include <cstring>

#include "attribute.h"
#include "engine.h"
#include "ratios.h"
#include "training.h"
#include "cluster.h"
#include "postprocess.h"
#include "utilities.h"
#include "disambiguate.h"

using std::list;
using std::string;


// TODO: add a private header so this can be unit tested
// This code is only used in this file
//#include "private.h"
string
remove_headtail_space(const string & s) {

    string::const_iterator istart = s.begin() , iend = s.end();
    while (istart != iend) {
        if (*istart != ' ') break;
        ++istart;
    }

    while (iend != istart) {
        --iend;
        if( *iend != ' ' ) {
            ++iend;
            break;
        }
    }
    string str_result(istart,iend);
    return str_result;
}




namespace BlockingConfiguration {
    using std::string;
    using std::vector;
    vector<BlockingConfiguration::cBlockingDetail> BlockingConfig;
    vector<string> active_similarity_attributes;
    const string ACTIVE_SIMILARITY_ATTRIBUTES_LABEL = "ACTIVE SIMILARITY ATTRIBUTES";
    std::auto_ptr<cBlocking_Operation> active_blocker_pointer;
    uint32_t name_cur_truncation = 0;
}

namespace EngineConfiguration {
    using std::vector;
    using std::string;

    const string WORKING_DIR_LABEL = "WORKING DIRECTORY";
    const string SOURCE_CSV_LABEL = "ORIGINAL CSV FILE";
    const string NUM_THREADS_LABEL = "NUMBER OF THREADS";
    const string WHETHER_GENERATE_STABLE_TRAINING_SETS_LABEL = "GENERATE STABLE TRAINING SETS";
    const string WHETHER_USE_AVAILABLE_RATIOS_DATABASE_LABEL = "USE AVAILABLE RATIOS DATABASE";
    const string THRESHOLDS_LABEL = "THRESHOLDS";
    const string NECESSARY_ATTRIBUTES_LABEL = "NECESSARY ATTRIBUTES";
    const string WHETHER_ADJUST_PRIOR_BY_FREQUENCY_LABEL = "ADJUST PRIOR BY FREQUENCY";
    const string DEBUG_MODE_LABEL = "DEBUG MODE";
    const string NUMBER_OF_TRAINING_PAIRS_LABEL = "NUMBER OF TRAINING PAIRS";
    const string STARTING_ROUND_LABEL = "STARTING ROUND";
    const string STARTING_FILE_LABEL = "STARTING FILE";
    const string POSTPROCESS_AFTER_EACH_ROUND_LABEL = "POSTPROCESS AFTER EACH ROUND";

    string working_dir;
    string source_csv_file;
    uint32_t number_of_threads;
    bool generate_stable_training_sets;
    bool use_available_ratios_database;
    vector < double > thresholds;
    vector < string > involved_columns;
    bool frequency_adjustment_mode;
    bool debug_mode;
    uint32_t number_of_training_pairs;
    uint32_t starting_round;
    string previous_disambiguation_result;
    bool postprocess_after_each_round;
}


int BlockingConfiguration::config_blocking(const char * filename, const string & module_id) {

   std::ostream & output = std::cout;
   BlockingConfiguration::config_blocking(filename, module_id, output);
}


int BlockingConfiguration::config_blocking(const char * filename, const string & module_id, std::ostream & outstream) {

    std::ostream & output = outstream;

    output << std::endl;
    output << "====================== " << module_id << " ===========================" << std::endl;
    output << "Reading Blocking Configuration from " << filename << " ... ..." << std::endl;
    output << std::endl;

    const char * delim = ":";
    const char * secondary_delim = ",";
    const char module_specifier = '[';
    const uint32_t delim_len = strlen(delim);
    const uint32_t secondary_delim_len = strlen(secondary_delim);
    std::ifstream infile(filename);

    if (!infile.good()) {
        output << "Blocking configuration file " << filename
               << " does not exist." << std::endl;
        return 1;
    }

    string linedata;
    size_t pos, prevpos;
    BlockingConfiguration::BlockingConfig.clear();

    while (getline(infile, linedata) && linedata.find(module_id) == string::npos);

    if (!infile) {
        output << "Cannot find module id: " << module_id << std::endl;
        return 2;
    }

    do {
        getline (infile, linedata);

        if (linedata.find(module_specifier) != string::npos)
            break;

        if (linedata.find(BlockingConfiguration::ACTIVE_SIMILARITY_ATTRIBUTES_LABEL) != string::npos)
            break;

        if (!infile) {
            output << "End of file." << std::endl;
            break;
        }

        pos = linedata.find(delim);
        if (pos == string::npos)
            continue;

        string lhs (linedata.c_str(), pos );
        const string columnname = remove_headtail_space(lhs);
        prevpos = pos + delim_len ;
        pos = linedata.find(delim, prevpos);
        const string dataindexstr = remove_headtail_space( linedata.substr(prevpos, pos - prevpos) );
        prevpos = pos + delim_len ;
        pos = linedata.find(secondary_delim, prevpos);
        const string beginstr = remove_headtail_space( linedata.substr(prevpos, pos - prevpos) );
        prevpos = pos + secondary_delim_len ;
        pos = linedata.find(secondary_delim, prevpos);
        const string ncharstr = remove_headtail_space( linedata.substr(prevpos, pos - prevpos) );
        prevpos = pos + secondary_delim_len ;
        pos = linedata.find(secondary_delim, prevpos);
        const string forwardstr = remove_headtail_space( linedata.substr(prevpos, pos - prevpos) );


        BlockingConfiguration::cBlockingDetail temp;
        temp.m_columnname = columnname;
        temp.m_dataindex = atoi ( dataindexstr.c_str());
        temp.m_begin = atoi ( beginstr.c_str());
        temp.m_nchar = atoi (ncharstr.c_str());
        temp.m_isforward = ( forwardstr == "false" ) ? false : true;


        output << "Column name = " << columnname
               << ", Data index = " << temp.m_dataindex
               << ", Beginning position = " << temp.m_begin
               << ", Number of chars = " << temp.m_nchar
               << ", Direction = " << (temp.m_isforward ? "true" : "false")
               << std::endl;

        if (columnname == cFirstname::static_get_class_name())
            BlockingConfiguration::name_cur_truncation = temp.m_nchar;

        BlockingConfiguration::BlockingConfig.push_back(temp);

    } while ( linedata.find(BlockingConfiguration::ACTIVE_SIMILARITY_ATTRIBUTES_LABEL) == string::npos);

    // now linedata contains similarity information.
    prevpos = 0;
    pos = linedata.find(delim);
    prevpos = pos + delim_len;
    BlockingConfiguration::active_similarity_attributes.clear();
    output << "Similarity Profiles include :";
    do {
        pos = linedata.find(secondary_delim, prevpos);
        const string tempname = remove_headtail_space( linedata.substr(prevpos, pos - prevpos) );
        prevpos = pos + secondary_delim_len ;
        if ( ! tempname.empty()) {
            BlockingConfiguration::active_similarity_attributes.push_back(tempname);
            output << tempname << ", ";
        }

    } while ( pos != string::npos );
    output << std::endl;


    vector<const StringManipulator *> pstring_oper;
    vector<string> columns_for_blocking;
    vector<uint32_t> data_indice_for_blocking;

    vector<BlockingConfiguration::cBlockingDetail>::const_iterator p = BlockingConfiguration::BlockingConfig.begin();
    for (; p != BlockingConfiguration::BlockingConfig.end(); ++p) {

        pstring_oper.push_back(p->m_psm);
        columns_for_blocking.push_back(p->m_columnname);
        data_indice_for_blocking.push_back(p->m_dataindex);

        StringTruncate * q = dynamic_cast< StringTruncate *>(p->m_psm);
        if (q == NULL) {
            std::cout << "------> ATTENTION: STRING OPERATOR OF "
                      << p->m_columnname
                      << " CANNOT BE DYNAMICALLY CAST. SKIP THIS OPERATION."
                      << std::endl;
        } else {
            q->set_truncater(p->m_begin, p->m_nchar, p->m_isforward);
        }
    }

    std::auto_ptr<cBlocking_Operation> bptr (
            new BlockByColumns (pstring_oper, columns_for_blocking, data_indice_for_blocking));
    BlockingConfiguration::active_blocker_pointer = bptr;

    return 0;
}


// Really important shit is hardwired via file-delimited namespacing,
// ugly things like are necessary. This needs to be rewired completely.
std::auto_ptr<cBlocking_Operation>
get_blocking_pointer() {
  return BlockingConfiguration::active_blocker_pointer;
}



bool
EngineConfiguration::config_engine(const char * filename, std::ostream & os) {

    os << "Reading Engine Configuration from " << filename << " ... ..." << std::endl;
    const char * delim = "=";
    const uint32_t delim_len = strlen(delim);
    std::ifstream infile(filename);
    if ( ! infile.good() ) {
        std::cout << "Engine configuration file " << filename << " does not exist." << std::endl;
        return false;
    }
    string linedata;
    size_t pos;
    int pieces_of_information = 0;
    const int must_have_information = 13;

    while ( getline (infile, linedata )) {
        pos = linedata.find(delim);
        if ( pos == string::npos )
            continue;
        string lhs (linedata.c_str(), pos );
        const string clean_lhs = remove_headtail_space(lhs);
        string rhs ( linedata.c_str() + pos + delim_len );
        string clean_rhs = remove_headtail_space(rhs);

        if ( clean_lhs == EngineConfiguration::WORKING_DIR_LABEL ) {
            EngineConfiguration::working_dir = clean_rhs;
            os << EngineConfiguration::WORKING_DIR_LABEL << " : "
                    << EngineConfiguration::working_dir << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::SOURCE_CSV_LABEL) {
            EngineConfiguration::source_csv_file = clean_rhs;
            os << EngineConfiguration::SOURCE_CSV_LABEL << " : "
                    << EngineConfiguration::source_csv_file << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::STARTING_FILE_LABEL) {
            EngineConfiguration::previous_disambiguation_result = clean_rhs;
            os << EngineConfiguration::STARTING_FILE_LABEL << " : "
                    << EngineConfiguration::previous_disambiguation_result << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::NUM_THREADS_LABEL) {
            EngineConfiguration::number_of_threads = atoi(clean_rhs.c_str());
            os << EngineConfiguration::NUM_THREADS_LABEL << " : "
                    <<  EngineConfiguration::number_of_threads << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::NUMBER_OF_TRAINING_PAIRS_LABEL){
            EngineConfiguration::number_of_training_pairs = atoi(clean_rhs.c_str());
            os << EngineConfiguration::NUMBER_OF_TRAINING_PAIRS_LABEL << " : "
                    <<  EngineConfiguration::number_of_training_pairs << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::STARTING_ROUND_LABEL){
            EngineConfiguration::starting_round = atoi(clean_rhs.c_str());
            os << EngineConfiguration::STARTING_ROUND_LABEL << " : "
                    <<  EngineConfiguration::starting_round << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::WHETHER_GENERATE_STABLE_TRAINING_SETS_LABEL ) {
            os << EngineConfiguration::WHETHER_GENERATE_STABLE_TRAINING_SETS_LABEL << " : ";
            if ( clean_rhs == "true" ) {
                EngineConfiguration::generate_stable_training_sets = true;
                os << " true ";
            }
            else if ( clean_rhs == "false") {
                EngineConfiguration::generate_stable_training_sets = false;
                os << " false ";
            }
            else
                throw cException_Other("Config Error: generate stable training sets.");
            os << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::WHETHER_USE_AVAILABLE_RATIOS_DATABASE_LABEL ){
            os << EngineConfiguration::WHETHER_USE_AVAILABLE_RATIOS_DATABASE_LABEL << " : ";
            if ( clean_rhs == "true" ) {
                EngineConfiguration::use_available_ratios_database = true;
                os << " true ";
            }
            else if ( clean_rhs == "false") {
                EngineConfiguration::use_available_ratios_database= false;
                os << " false ";
            }
            else
                throw cException_Other("Config Error: use available ratios database");
            os << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::POSTPROCESS_AFTER_EACH_ROUND_LABEL ){
            os << EngineConfiguration::POSTPROCESS_AFTER_EACH_ROUND_LABEL << " : ";
            if ( clean_rhs == "true" ) {
                EngineConfiguration::postprocess_after_each_round = true;
                os << " true ";
            }
            else if ( clean_rhs == "false") {
                EngineConfiguration::postprocess_after_each_round = false;
                os << " false ";
            }
            else
                throw cException_Other("Config Error: post processing");
            os << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::WHETHER_ADJUST_PRIOR_BY_FREQUENCY_LABEL ){
            os << EngineConfiguration::WHETHER_ADJUST_PRIOR_BY_FREQUENCY_LABEL<< " : ";
            if ( clean_rhs == "true" ) {
                EngineConfiguration::frequency_adjustment_mode = true;
                os << " true ";
            }
            else if ( clean_rhs == "false") {
                EngineConfiguration::frequency_adjustment_mode = false;
                os << " false";
            }
            else
                throw cException_Other("Config Error: frequency adjustment mode");
            os << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::DEBUG_MODE_LABEL ){
            os << EngineConfiguration::DEBUG_MODE_LABEL<< " : ";
            if ( clean_rhs == "true" ) {
                EngineConfiguration::debug_mode = true;
                os << " true ";
            }
            else if ( clean_rhs == "false") {
                EngineConfiguration::debug_mode = false;
                os << " false ";
            }
            else
                throw cException_Other("Config Error: debug mode");
            os << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::THRESHOLDS_LABEL ) {
            const char * strdelim = ", ";
            const uint32_t strdelim_len = strlen(strdelim);
            size_t strpos = 0, strprevpos = 0;
            EngineConfiguration::thresholds.clear();
            os << EngineConfiguration::THRESHOLDS_LABEL << " : ";
            do {
                strpos = clean_rhs.find(strdelim, strprevpos);
                string temp = clean_rhs.substr(strprevpos, strpos - strprevpos );
                double t = atof(temp.c_str());
                if ( t <= 0 || t >= 1 )
                    throw cException_Other("Config Error: thresholds");
                EngineConfiguration::thresholds.push_back(t);
                os << t << strdelim;
                strprevpos = strpos + strdelim_len;
            }
            while ( strpos != string::npos );
            os << std::endl;
        }

        else if ( clean_lhs == EngineConfiguration::NECESSARY_ATTRIBUTES_LABEL ) {
            const char * strdelim = ", ";
            const uint32_t strdelim_len = strlen(strdelim);
            size_t strpos = 0, strprevpos = 0;
            EngineConfiguration::involved_columns.clear();
            os << EngineConfiguration::NECESSARY_ATTRIBUTES_LABEL << " : ";
            do {
                strpos = clean_rhs.find(strdelim, strprevpos);
                string temp = clean_rhs.substr(strprevpos, strpos - strprevpos );
                EngineConfiguration::involved_columns.push_back(temp);
                os << temp << strdelim;
                strprevpos = strpos + strdelim_len;
            }
            while ( strpos != string::npos );
            os << std::endl;
        }
        else {
            continue;
        }

        ++pieces_of_information;
    }
    if (must_have_information == pieces_of_information)
        return true;
    else
        return false;
}


int
disambiguate_main(std::string & engineconf, std::string & blockingconf) {

// This is the initial code for running the disambiguations, leave it
// in until it can be totally factored out.
/*
    std::cout << std::endl;
    std::cout << "====================== STARTING DISAMBIGUATION ===========================" << std::endl;
    std::cout << "__FILE__:__LINE__" << __FILE__ << ":" << __LINE__ << std::endl;
    std::cout << std::endl;

    int choice;
    std::cout << "Choose one option: 1. Disambiguation. 2. Calculate Out-Of-Cluster Density. Choice = ";
    std::cin >> choice;
*/
    int choice = 1;

    string enginefile ;
    string blockingfile ;
    switch (choice) {

    case 1:
        //enginefile = check_file_existence("Disambiguation Engine Configuration.");
        //blockingfile = check_file_existence("Disambiguation Blocking Configuration.");
        //Full_Disambiguation(enginefile.c_str(), blockingfile.c_str());
        Full_Disambiguation(engineconf.c_str(), blockingconf.c_str());
        break;

    case 2: {
        enginefile = check_file_existence("Disambiguation Engine Configuration.");
        const string upper = check_file_existence("upper bound.");
        const string lower = check_file_existence("lower bound.");
        const string ratio = check_file_existence("Ratios database.");
        string oocd_res;
        std::cout << "Out-of-cluster density output file :";
        std::cin >> oocd_res;
        std::ofstream ofile ( oocd_res.c_str());

        if ( ! ofile.good() )
            throw cException_Other("Out-of-cluster density output file error.");

        if ( ! EngineConfiguration::config_engine(enginefile.c_str(), std::cout ) )
            throw cException_Other("Engine Configuration is not complete!");

        list <Record> all_records;
        const vector <string> column_vec = EngineConfiguration::involved_columns;
        bool is_success = fetch_records_from_txt(all_records, EngineConfiguration::source_csv_file.c_str(), column_vec);
        if (not is_success) return 1;

        // TODO: document what this block achieves
        map <string, const Record *> uid_dict;
        const string uid_identifier = cUnique_Record_ID::static_get_class_name();
        create_btree_uid2record_pointer(uid_dict, all_records, uid_identifier);

        map<const Record *, RecordPList, cSort_by_attrib> patent_tree(cPatent::static_get_class_name());
        build_patent_tree(  patent_tree , all_records ) ;
        Cluster::set_reference_patent_tree_pointer( patent_tree);
        list < const Record *> all_rec_pointers;
        for ( list<Record>::const_iterator p = all_records.begin(); p != all_records.end(); ++p )
          all_rec_pointers.push_back(&(*p));
        // cAssignee::configure_assignee(all_rec_pointers);
        
        ClusterSet up, low;
        up.read_from_file(upper.c_str(), uid_dict);
        low.read_from_file(lower.c_str(), uid_dict);
        const cRatios ratiodb ( ratio.c_str());
        out_of_cluster_density(up, low, ratiodb, ofile);
        break;
    }

    default:
        std::cerr << "Invalid selection. Program exits." << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "====================== END OF DISAMBIGUATION =============================" << std::endl;
    std::cout << std::endl;
    return 0;
}



int
Full_Disambiguation( const char * EngineConfigFile, const char * BlockingConfigFile ) {

    if (!EngineConfiguration::config_engine(EngineConfigFile, std::cout)) {
        throw cException_Other("Engine Configuration is not complete!");
    }

    const bool train_stable               = EngineConfiguration::generate_stable_training_sets;
    const bool use_available_ratios       = EngineConfiguration::use_available_ratios_database;
    const string working_dir              = EngineConfiguration::working_dir;
    const vector<double> threshold_vec    = EngineConfiguration::thresholds ;
    const uint32_t num_threads            = EngineConfiguration::number_of_threads;
    const vector<string> column_vec       = EngineConfiguration::involved_columns;
    const uint32_t limit                  = EngineConfiguration::number_of_training_pairs;
    bool frequency_adjust_mode            = EngineConfiguration::frequency_adjustment_mode;
    bool debug_mode                       = EngineConfiguration::debug_mode;
    const uint32_t starting_round         = EngineConfiguration::starting_round;
    const uint32_t buff_size = 512;

   /**
    * Read in the CSV file containing consolidated inventor-patent instances.
    * This file is typically named "invpat.csv", but the filename is
    * arbitrary. The data must have the following characteristics:
    * 1. Header row of csv-parseable strings as column labels.
    * 2. Valid comma-separated fields in each row.
    */
    list<Record> all_records;
    char recordsfile[buff_size];
    sprintf(recordsfile, "%s", EngineConfiguration::source_csv_file.c_str());
    bool is_success = fetch_records_from_txt(all_records, recordsfile, column_vec);
    if (not is_success) return 1;


    // There is a function for this elsewhere, and it should be used instead of
    // the following code. Also, if possible, all the initialization code like
    // the following should go into an initialization function.
    // TODO: Refactor
    // TODO: Find a way to unit test this.
    // There is another version of this in training.cpp and in the build_patent_tree
    // function.
    RecordPList all_rec_pointers;
    for (list<Record>::const_iterator p = all_records.begin(); p != all_records.end(); ++p) {
        all_rec_pointers.push_back(&(*p));
    }


    // cAssignee::configure_assignee(all_rec_pointers);
    //std::cout << "Passed configuring assignees..." << std::endl;

    //patent stable
    const string training_stable [] = {working_dir + "/xset03_stable.txt",
                                       working_dir + "/tset02_stable.txt"};
    const vector<string> training_stable_vec (training_stable, training_stable + sizeof(training_stable)/sizeof(string));
    if (train_stable) {
        make_stable_training_sets_by_personal (all_records, limit, training_stable_vec);
    }

    //std::cout << "Stable training sets made..." << std::endl;

    // This is a blocking typedef, RecordIndex, ratios.h:21
    map<string, const Record *> uid_dict;
    const string uid_identifier = cUnique_Record_ID::static_get_class_name();
    create_btree_uid2record_pointer(uid_dict, all_records, uid_identifier);

    //train patent info
    // TODO: check to see whether uid_dict is modified.
    // It looks like uid_dict comes in as const, which means
    // the patentinfo initialization can be moved to where
    // it's being used. This will help refactoring.
    cRatioComponent patentinfo(uid_dict, string("Patent") );

    // `record_pointers` are not being used.
    // TODO: Get rid of this declaration.
    RecordPList record_pointers;


    char xset01[buff_size];
    char tset05[buff_size];
    char ratiofile[buff_size];
    char matchfile[buff_size];
    // TODO: Move stat_patent declaration further down in the file, closer
    // to where it's being used. This will allow future refactoring.
    char stat_patent[buff_size];
    char stat_personal[buff_size];
    char oldmatchfile[buff_size];
    char debug_block_file[buff_size];
    char network_file[buff_size];
    char postprocesslog[buff_size];
    char prior_save_file[buff_size];
    char roundstr[buff_size];

    sprintf(oldmatchfile,"%s", EngineConfiguration::previous_disambiguation_result.c_str() );


    // TODO: tying a general operation in configuration file to a specific
    // operation in the implementation is a wrong abstraction. Post-processing
    // is general, but network_clustering is very specific.
    bool network_clustering = EngineConfiguration::postprocess_after_each_round;

    if (debug_mode) network_clustering = false;



    // TODO: move this declaration to where it's being used, such that it
    // can be refactored out soon.
    cRatioComponent personalinfo(uid_dict, string("Personal") );

    const uint32_t num_coauthors_to_group = 2;

    // BOGOSITY: This needs to be set before the "patent tree" is initialized so
    // that clusters will instantiate. Bogus.
    cBlocking_Operation_By_Coauthors blocker_coauthor(all_rec_pointers, num_coauthors_to_group);

    // TODO: Refactor
    std::cout << "Reconfiguring ..." << std::endl;
    // const Reconfigurator_AsianNames corrector_asiannames;
    // std::for_each (all_rec_pointers.begin(), all_rec_pointers.end(), corrector_asiannames);
    Reconfigurator_Coauthor corrector_coauthor (blocker_coauthor.get_patent_tree());
    std::for_each (all_rec_pointers.begin(), all_rec_pointers.end(), corrector_coauthor);
    std::cout << "Reconfiguration done." << std::endl;
    ///////// End refactor

    Cluster::set_reference_patent_tree_pointer(blocker_coauthor.get_patent_tree());

    vector<string> prev_train_vec;

    // Moved down to where it's being used.
    //uint32_t firstname_prev_truncation = BlockingConfiguration::firstname_cur_truncation;

    const string module_prefix = "Round ";
    string module_name ;
    // `is_blockingconfig_success` needs to be a boolean
    int is_blockingconfig_success;

    uint32_t round = starting_round;
    while (true) {

        sprintf(roundstr, "%d", round);
        module_name = module_prefix + roundstr;

        // Successive reads of the blocking configuration file.
        is_blockingconfig_success = BlockingConfiguration::config_blocking(BlockingConfigFile, module_name);
        if (is_blockingconfig_success != 0) break;

        // TODO: Refactor all these into a utility class.
        sprintf(xset01, "%s/xset01_%d.txt", working_dir.c_str(), round);
        sprintf(tset05, "%s/tset05_%d.txt", working_dir.c_str(), round);
        sprintf(ratiofile, "%s/ratio_%d.txt", working_dir.c_str(), round);
        sprintf(matchfile, "%s/newmatch_%d.txt", working_dir.c_str(), round);
        sprintf(stat_patent, "%s/stat_patent_%d.txt", working_dir.c_str(), round);
        sprintf(stat_personal, "%s/stat_personal_%d.txt", working_dir.c_str(), round);
        sprintf(debug_block_file, "%s/debug_block_%d.txt", working_dir.c_str(), round);
        sprintf(network_file, "%s/network_%d.txt", working_dir.c_str(), round);
        sprintf(postprocesslog, "%s/postprocesslog_%d.txt", working_dir.c_str(), round);
        sprintf(prior_save_file, "%s/prior_saved_%d.txt", working_dir.c_str(), round);

        Record::activate_comparators_by_name(BlockingConfiguration::active_similarity_attributes);
        //now training
        //match.output_list(record_pointers);
        bool matching_mode = true;
        ClusterInfo match (uid_dict, matching_mode, frequency_adjust_mode, debug_mode);
        match.set_thresholds(threshold_vec);


        const string training_changable [] = { xset01, tset05 };
        const vector<string> training_changable_vec (training_changable,
            training_changable + sizeof(training_changable)/sizeof(string));

        if (1 == round) {

            // TODO: Refactor this block.
            vector<string> presort_columns;
            StringRemainSame operator_no_change;
            presort_columns.push_back(cFirstname::static_get_class_name());
            // presort_columns.push_back(cLastname::static_get_class_name());
            // presort_columns.push_back(cAssignee::static_get_class_name());
            presort_columns.push_back(cStreet::static_get_class_name());
            // presort_columns.push_back(cCity::static_get_class_name());
            presort_columns.push_back(cCountry::static_get_class_name());
            //presort_columns.push_back(cClass::static_get_class_name());

            const vector<const StringManipulator *> presort_strman(presort_columns.size(),
                &operator_no_change);
            const vector<uint32_t> presort_data_indice(presort_columns.size(), 0);

            const BlockByColumns presort_blocker(
                presort_strman, presort_columns, presort_data_indice);

            match.preliminary_consolidation(presort_blocker, all_rec_pointers);
            match.output_current_comparision_info(oldmatchfile);
        }


        uint32_t name_prev_truncation = BlockingConfiguration::name_cur_truncation;
        cFirstname::set_truncation(name_prev_truncation, BlockingConfiguration::name_cur_truncation);
        name_prev_truncation = BlockingConfiguration::name_cur_truncation;
        std::cout << "Resetting blocking" << std:endl;
        match.reset_blocking(*BlockingConfiguration::active_blocker_pointer, oldmatchfile);

        if (network_clustering) {
          std::cout << "In network clustering block" << std::endl;

            // TODO: Try to refactor this block.
            blocker_coauthor.build_uid2uinv_tree(match);
            ClusterSet cs;
            //cs.convert_from_ClusterInfo(&match);
            cs.read_from_file(oldmatchfile, uid_dict);
            post_polish(cs, blocker_coauthor.get_uid2uinv_tree(),
                        blocker_coauthor.get_patent_tree(), string(postprocesslog));
            cs.output_results(network_file);
            match.reset_blocking( * BlockingConfiguration::active_blocker_pointer, network_file);
        }


        if (!use_available_ratios) {
          std::cout << "Generating ratios" << std::endl;

            const BlockByColumns & blocker_ref =
                    dynamic_cast<BlockByColumns &> (*BlockingConfiguration::active_blocker_pointer);
            make_changable_training_sets_by_patent(all_rec_pointers, blocker_ref.get_blocking_attribute_names(),
                    blocker_ref.get_blocking_string_manipulators(), limit, training_changable_vec);
        }

        const cRatios * ratio_pointer;
        if (!use_available_ratios) {
            // TODO: Try to refactor all this.
            personalinfo.prepare(training_changable_vec.at(0).c_str(), training_changable_vec.at(1).c_str());
            patentinfo.prepare(training_stable_vec.at(0).c_str(), training_stable_vec.at(1).c_str());

            personalinfo.stats_output(stat_personal);
            patentinfo.stats_output(stat_patent);

            vector <const cRatioComponent *> component_vector;
            component_vector.push_back(&patentinfo);
            component_vector.push_back(&personalinfo);

            ratio_pointer = new cRatios (component_vector, ratiofile, all_records.front());
        } else {
            ratio_pointer = new cRatios (ratiofile);
        }

        Cluster::set_ratiomap_pointer(*ratio_pointer);

        // now disambiguate
        Record::clean_member_attrib_pool();
        // ClusterInfo.disambiguate
        match.disambiguate(*ratio_pointer, num_threads, debug_block_file, prior_save_file);
        delete ratio_pointer;
        match.output_current_comparision_info(matchfile);

        strcpy (oldmatchfile, matchfile);
        ++round;
    }


    // post-processing now
    // `is_blockingconfig_success` should be a boolean
    // TODO: Find a maintainable way to handle this
    if (is_blockingconfig_success == 2) {
        std::cout << "Final post processing ... ..." << std::endl;
        one_step_postprocess( all_records, oldmatchfile, ( string(working_dir) + "/final.txt").c_str() );
    }

    return 0;
}
