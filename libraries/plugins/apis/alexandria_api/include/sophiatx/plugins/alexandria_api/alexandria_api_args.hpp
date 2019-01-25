#pragma once

#include <sophiatx/plugins/alexandria_api/alexandria_api_objects.hpp>
#include <sophiatx/plugins/block_api/block_api_args.hpp>
#include <sophiatx/plugins/database_api/database_api_args.hpp>
#include <sophiatx/plugins/account_history_api/account_history_args.hpp>

#include <sophiatx/protocol/types.hpp>

namespace sophiatx { namespace plugins { namespace alexandria_api {

/**
 * info
 */
// variant                             info();
struct info_return {
   variant	info;
};

/**
 * about
 */
// variant_object                      about();
struct about_return {
   variant_object	about;
};

/**
 * get_block
 */
// optional<database_api::api_signed_block_object> get_block(uint32_t num);
struct get_block_args {
   uint32_t	num;
};
struct get_block_return {
   optional<database_api::api_signed_block_object>	block;
};

/**
 * get_ops_in_block
 */
// vector<alexandria_api::api_operation_object> get_ops_in_block(uint32_t block_num,bool only_virtual);
struct get_ops_in_block_args {
   uint32_t	block_num;
   bool	only_virtual;
};
struct get_ops_in_block_return {
   vector<alexandria_api::api_operation_object>	ops_in_block;
};

/**
 * get_feed_history
 */
// alexandria_api::api_feed_history_object get_feed_history(string symbol);
struct get_feed_history_args {
   string	symbol;
};
struct get_feed_history_return {
   alexandria_api::api_feed_history_object	feed_history;
};

/**
 * get_active_witnesses
 */
// vector<account_name_type> get_active_witnesses();
struct get_active_witnesses_return {
   vector<account_name_type>	active_witnesses;
};

/**
 * get_account
 */
// vector<alexandria_api::api_account_object> get_account(string account_name);
struct get_account_args {
   string	account_name;
};
struct get_account_return {
   vector<alexandria_api::api_account_object>	account;
};


/**
 * get_accounts
 */
struct get_accounts_args {
   std::vector<account_name_type> account_names;
};
struct get_accounts_return {
   std::vector<extended_account>	accounts;
};

/**
 * get_transaction
 */
// annotated_signed_transaction get_transaction(transaction_id_type tx_id);
struct get_transaction_args {
   transaction_id_type	tx_id;
};
struct get_transaction_return {
   annotated_signed_transaction	tx;
};

/**
 * create_account
 */
// operation create_account(string creator,string seed,string json_meta,public_key_type owner,public_key_type active,public_key_type memo);
struct create_account_args {
   string	creator;
   string	name_seed;
   string	json_meta;
   public_key_type	owner;
   public_key_type	active;
   public_key_type	memo;
};
struct create_account_return {
   operation	op;
};

/**
 * update_account
 */
// operation update_account(string accountname,string json_meta,public_key_type owner,public_key_type active,public_key_type memo);
struct update_account_args {
   string	account_name;
   string	json_meta;
   public_key_type	owner;
   public_key_type	active;
   public_key_type	memo;
};
struct update_account_return {
   operation	op;
};

/**
 * update_account_auth
 */
// operation update_account(string accountname, authority_type type, authority new_authority);
struct update_account_auth_args {
   string	account_name;
   authority_type    type;
   authority	new_authority;
};
struct update_account_auth_return {
   operation	op;
};

/**
 * delete_account
 */
// operation delete_account(string account_name);
struct delete_account_args {
   string	account_name;
};
struct delete_account_return {
   operation	op;
};

/**
 * get_transaction_id
 */
// transaction_id_type get_transaction_id(signed_transaction tx){ return trx.id();
struct get_transaction_id_args {
   signed_transaction	tx;
};
struct get_transaction_id_return {
   transaction_id_type	tx_id;
};

/**
 * list_witnesses
 */
// vector<account_name_type> list_witnesses(string start,uint32_t limit);
struct list_witnesses_args {
   string	start;
   uint32_t	limit;
};
struct list_witnesses_return {
   vector<account_name_type>	witnesses;
};

/**
 * list_witnesses_by_vote
 */
// vector<alexandria_api::api_witness_object> list_witnesses_by_vote(string name,uint32_t limit);
struct list_witnesses_by_vote_args {
   string	name;
   uint32_t	limit;
};
struct list_witnesses_by_vote_return {
   vector<alexandria_api::api_witness_object>	witnesses_by_vote;
};

/**
 * get_witness
 */
// optional<alexandria_api::api_witness_object> get_witness(string owner_account);
struct get_witness_args {
   string	owner_account;
};
struct get_witness_return {
   optional<alexandria_api::api_witness_object>	witness;
};

/**
 * update_witness
 */
// operation update_witness(string witness_name,string url,public_key_type block_signing_key,chain_properties props);
struct update_witness_args {
   string	witness_account_name;
   string	url;
   public_key_type	block_signing_key;
   chain_properties	props;
};
struct update_witness_return {
   operation	op;
};

/**
 * stop_witness
 */
// operation stop_witness(string witness_name);
struct stop_witness_args {
   string	witness_account_name;
};
struct stop_witness_return {
   operation	op;
};

/**
 * set_voting_proxy
 */
// operation set_voting_proxy(string account_to_modify,string proxy);
struct set_voting_proxy_args {
   string	account_to_modify;
   string	proxy;
};
struct set_voting_proxy_return {
   operation	op;
};

/**
 * vote_for_witness
 */
// operation vote_for_witness(string voting_account,string witness_to_vote_for,bool approve);
struct vote_for_witness_args {
   string	voting_account;
   string	witness_to_vote_for;
   bool	approve;
};
struct vote_for_witness_return {
   operation	op;
};

/**
 * transfer
 */
// operation transfer(string from,string to,asset amount,string memo);
struct transfer_args {
   string	from;
   string	to;
   asset	   amount;
   string	memo;
};
struct transfer_return {
   operation	op;
};

/**
 * transfer_to_vesting
 */
// operation transfer_to_vesting(string from,string to,asset amount);
struct transfer_to_vesting_args {
   string	from;
   string	to;
   asset	amount;
};
struct transfer_to_vesting_return {
   operation	op;
};

/**
 * withdraw_vesting
 */
// operation withdraw_vesting(string from,asset vesting_shares);
struct withdraw_vesting_args {
   string	from;
   asset	vesting_shares;
};
struct withdraw_vesting_return {
   operation	op;
};

/**
 * get_owner_history
 */
// vector<database_api::api_owner_authority_history_object> get_owner_history(string account);
struct get_owner_history_args {
   string	account;
};
struct get_owner_history_return {
   vector<database_api::api_owner_authority_history_object>	owner_history;
};

/**
 * create_application
 */
// operation create_application(string author,string app_name,string url,string meta_data,uint8_t price_param);
struct create_application_args {
   string	author;
   string	app_name;
   string	url;
   string	meta_data;
   uint8_t	price_param;
};
struct create_application_return {
   operation	op;
};

/**
 * update_application
 */
// operation update_application(string author,string app_name,string new_author,string url,string meta_data,uint8_t price_param);
struct update_application_args {
   string	author;
   string	app_name;
   string	new_author;
   string	url;
   string	meta_data;
   uint8_t	price_param;
};
struct update_application_return {
   operation	op;
};

/**
 * delete_application
 */
// operation delete_application(string author,string app_name);
struct delete_application_args {
   string	author;
   string	app_name;
};
struct delete_application_return {
   operation	op;
};

/**
 * buy_application
 */
// operation buy_application(string buyer,int64_t app_id);
struct buy_application_args {
   string	buyer;
   int64_t	app_id;
};
struct buy_application_return {
   operation	op;
};

/**
 * cancel_application_buying
 */
// operation cancel_application_buying(string app_owner,string buyer,int64_t app_id);
struct cancel_application_buying_args {
   string	app_owner;
   string	buyer;
   int64_t	app_id;
};
struct cancel_application_buying_return {
   operation	op;
};

/**
 * get_application_buyings
 */
// vector<alexandria_api::api_application_buying_object>  get_application_buyings(string name,string search_type,uint32_t count);
struct get_application_buyings_args {
   string	name;
   string	search_type;
   uint32_t	count;
};
struct get_application_buyings_return {
   vector<alexandria_api::api_application_buying_object>	application_buyings;
};

/**
 * make_custom_json_operation
 */
// operation make_custom_json_operation(uint32_t app_id,string from,vector<string> to,string json);
struct make_custom_json_operation_args {
   uint32_t	app_id;
   string	from;
   vector<string>	to;
   string	json;
};
struct make_custom_json_operation_return {
   operation	op;
};

/**
 * make_custom_binary_operation
 */
// operation make_custom_binary_operation(uint32_t app_id,string from,vector<string> to,string data);
struct make_custom_binary_operation_args {
   uint32_t	app_id;
   string	from;
   vector<string>	to;
   string	data;
};
struct make_custom_binary_operation_return {
   operation	op;
};

/**
 * broadcast_transaction
 */
// annotated_signed_transaction broadcast_transaction(signed_transaction tx);
struct broadcast_transaction_args {
   signed_transaction	tx;
};
struct broadcast_transaction_return {
   annotated_signed_transaction	tx;
};

/**
 * create_transaction
 */
// signed_transaction create_transaction(vector<operation> op_vec);
struct create_transaction_args {
   vector<operation>	op_vec;
};
struct create_transaction_return {
   signed_transaction	tx;
};

/**
 * create_simple_transaction
 */
// signed_transaction create_simple_transaction(operation op);
struct create_simple_transaction_args {
   operation	op;
};
struct create_simple_transaction_return {
   signed_transaction	simple_tx;
};

/**
 * get_applications
 */
// vector<alexandria_api::api_application_object>  get_applications(vector<string> names);
struct get_applications_args {
   vector<string>	names;
};
struct get_applications_return {
   vector<alexandria_api::api_application_object>	applications;
};

/**
 * get_applications_by_ids
 */
// vector<alexandria_api::api_application_object>  get_applications_by_ids(vector<uint32_t> ids);
struct get_applications_by_ids_args {
   vector<uint32_t>	ids;
};
struct get_applications_by_ids_return {
   vector<alexandria_api::api_application_object>	applications;
};

/**
 * get_transaction_digest
 */
// digest_type get_transaction_digest(signed_transaction tx);
struct get_transaction_digest_args {
   signed_transaction	tx;
};
struct get_transaction_digest_return {
   digest_type	tx_digest;
};

/**
 * add_signature
 */
// signed_transaction add_signature(signed_transaction tx,fc::ecc::compact_signature signature);
struct add_signature_args {
   signed_transaction	tx;
   fc::ecc::compact_signature	signature;
};
struct add_signature_return {
   signed_transaction	signed_tx;
};

/**
 * add_fee
 */
// operation add_fee(operation op,asset fee);
struct add_fee_args {
   operation	op;
   asset	fee;
};
struct add_fee_return {
   operation	op;
};

/**
 * sign_digest
 */
// fc::ecc::compact_signature sign_digest(digest_type digest,string pk);
struct sign_digest_args {
   digest_type	digest;
   string	pk;
};
struct sign_digest_return {
   fc::ecc::compact_signature	signed_digest;
};

/**
 * send_and_sign_operation
 */
// annotated_signed_transaction send_and_sign_operation(operation op,string pk);
struct send_and_sign_operation_args {
   operation	op;
   string	pk;
};
struct send_and_sign_operation_return {
   annotated_signed_transaction	signed_tx;
};

/**
 * send_and_sign_transaction
 */
// annotated_signed_transaction send_and_sign_transaction(signed_transaction tx,string pk);
struct send_and_sign_transaction_args {
   signed_transaction	tx;
   string	pk;
};
struct send_and_sign_transaction_return {
   annotated_signed_transaction	signed_tx;
};

/**
 * verify_signature
 */
// bool verify_signature(digest_type digest,public_key_type pub_key,fc::ecc::compact_signature signature);
struct verify_signature_args {
   digest_type	digest;
   public_key_type	pub_key;
   fc::ecc::compact_signature	signature;
};
struct verify_signature_return {
   bool	signature_valid;
};

/**
 * generate_key_pair
 */
// key_pair generate_key_pair();
struct generate_key_pair_return {
   key_pair_st	key_pair;
};

/**
 * generate_key_pair_from_brain_key
 */
// key_pair generate_key_pair_from_brain_key(string brain_key);
struct generate_key_pair_from_brain_key_args {
   string	brain_key;
};
struct generate_key_pair_from_brain_key_return {
   key_pair_st	key_pair;
};

/**
 * get_public_key
 */
// public_key_type get_public_key(string private_key);
struct get_public_key_args {
   string	private_key;
};
struct get_public_key_return {
   public_key_type	public_key;
};

/**
 * from_base64
 */
// string from_base64(string data);
struct from_base64_args {
   string	data;
};
struct from_base64_return {
   string	str;
};

/**
 * to_base64
 */
// string to_base64(string data);
struct to_base64_args {
   string	data;
};
struct to_base64_return {
   string	base64_str;
};

/**
 * encrypt_data
 */
// string encrypt_data(string data,public_key_type public_key,string private_key);
struct encrypt_data_args {
   string	data;
   public_key_type	public_key;
   string	private_key;
};
struct encrypt_data_return {
   string	encrypted_data;
};

/**
 * decrypt_data
 */
// string decrypt_data(string data,public_key_type public_key,string private_key);
struct decrypt_data_args {
   string	data;
   public_key_type	public_key;
   string	private_key;
};
struct decrypt_data_return {
   string	decrypted_data;
};

/**
 * account_exist
 */
// bool account_exist(string account_name);
struct account_exist_args {
   string	account_name;
};
struct account_exist_return {
   bool	account_exist;
};

#ifdef ABAP_INTERFACE
/**
 * get_account_history
 */
// vector<alexandria_api::api_operation_object> get_account_history(string account,uint32_t from,uint32_t limit);
struct get_account_history_args {
   string	account;
   uint32_t	from;
   uint32_t	limit;
};
struct get_account_history_return {
   vector<alexandria_api::api_operation_object>	account_history;
};

/**
 * get_received_documents
 */
// vector<alexandria_api::api_received_object>  get_received_documents(uint32_t app_id,string account_name,string search_type,string start,uint32_t count);
struct get_received_documents_args {
   uint32_t	app_id;
   string	account_name;
   string	search_type;
   string	start;
   uint32_t	count;
};
struct get_received_documents_return {
   vector<alexandria_api::api_received_object>	received_documents;
};

#else
/**
 * get_account_history
 */
// map<uint32_t,alexandria_api::api_operation_object> get_account_history(string account,uint32_t from,uint32_t limit);
struct get_account_history_args {
   string	account;
   uint32_t	start;
   uint32_t	limit;
};
struct get_account_history_return {
   map<uint32_t,alexandria_api::api_operation_object>	account_history;
};

/**
 * get_received_documents
 */
// map<uint64_t,alexandria_api::api_received_object>  get_received_documents(uint32_t app_id,string account_name,string search_type,string start,uint32_t count);
struct get_received_documents_args {
   uint32_t	app_id;
   string	account_name;
   string	search_type;
   string	start;
   uint32_t	count;
};
struct get_received_documents_return {
   map<uint64_t,alexandria_api::api_received_object>	received_documents;
};
#endif

/**
 * get_active_authority
 */
// authority get_active_authority(string account_name);
struct get_active_authority_args {
   string	account_name;
};
struct get_active_authority_return {
   authority	active_authority;
};

/**
 * get_owner_authority
 */
// authority get_owner_authority(string account_name);
struct get_owner_authority_args {
   string	account_name;
};
struct get_owner_authority_return {
   authority	owner_authority;
};

/**
 * get_memo_key
 */
// public_key_type get_memo_key(string account_name);
struct get_memo_key_args {
   string	account_name;
};
struct get_memo_key_return {
   public_key_type	memo_key;
};

/**
 * get_account_balance
 */
// int64_t get_account_balance(string account_name);
struct get_account_balance_args {
   string	account_name;
};
struct get_account_balance_return {
   int64_t	account_balance;
};

/**
 * get_vesting_balance
 */
// int64_t get_vesting_balance(string account_name);
struct get_vesting_balance_args {
   string	account_name;
};
struct get_vesting_balance_return {
   int64_t	vesting_balance;
};

/**
 * create_simple_authority
 */
// authority create_simple_authority(public_key_type pub_key);
struct create_simple_authority_args {
   public_key_type	pub_key;
};
struct create_simple_authority_return {
   authority	simple_authority;
};

/**
 * create_simple_multisig_authority
 */
// authority create_simple_multisig_authority(vector<public_key_type> pub_keys,uint32_t required_signatures);
struct create_simple_multisig_authority_args {
   vector<public_key_type>	pub_keys;
   uint32_t	required_signatures;
};
struct create_simple_multisig_authority_return {
   authority	simple_multisig_authority;
};

/**
 * create_simple_managed_authority
 */
// authority create_simple_managed_authority(string managing_account);
struct create_simple_managed_authority_args {
   string	managing_account;
};
struct create_simple_managed_authority_return {
   authority	simple_managed_authority;
};

/**
 * create_simple_multisig_managed_authority
 */
// authority create_simple_multisig_managed_authority(vector<string> managing_accounts,uint32_t required_signatures);
struct create_simple_multisig_managed_authority_args {
   vector<string>	managing_accounts;
   uint32_t	required_signatures;
};
struct create_simple_multisig_managed_authority_return {
   authority	simple_multisig_managed_authority;
};

/**
 * get_account_name_from_seed
 */
// string get_account_name_from_seed(string seed);
struct get_account_name_from_seed_args {
   string	seed;
};
struct get_account_name_from_seed_return {
   string	account_name;
};

/**
 * get_required_signatures
 */
// set<public_key_type> get_required_signatures(signed_transaction tx);
struct get_required_signatures_args {
   signed_transaction	tx;
};
struct get_required_signatures_return {
   set<public_key_type>	required_signatures;
};

/**
 * calculate_fee
 */
// asset calculate_fee(operation op,asset_symbol_type symbol);
struct calculate_fee_args {
   operation	op;
   asset_symbol_type	symbol;
};
struct calculate_fee_return {
   asset	fee;
};

/**
 * fiat_to_sphtx
 */
// asset fiat_to_sphtx(asset fiat);
struct fiat_to_sphtx_args {
   asset	fiat;
};
struct fiat_to_sphtx_return {
   asset	sphtx;
};

/**
 * custom_object_subscription
 */
// uint64_t custom_object_subscription(std::function<void(variant)> callback,uint32_t app_id,string account_name,string search_type,uint64_t start);
struct custom_object_subscription_args {
   //std::function<void(variant)>	callback;    // TODO: check validity
   uint64_t return_id;
   uint32_t	app_id;
   string	account_name;
   string	search_type;
   uint64_t	start;
};
struct custom_object_subscription_return {
   uint64_t	subscription;
};

/**
 * sponsor_account_fees
 */
// operation sponsor_account_fees(string sponsoring_account,string sponsored_account,bool is_sponsoring);
struct sponsor_account_fees_args {
   string	sponsoring_account;
   string	sponsored_account;
   bool	   is_sponsoring;
};
struct sponsor_account_fees_return {
   operation	op;
};

/**
 *  get_key_references
 */
struct get_key_references_args {
   std::vector< sophiatx::protocol::public_key_type > keys;
};
struct get_key_references_return {
   std::vector< std::vector< account_name_type > > accounts;
};

/**
 *  get_version
 */
struct get_version_return {
   get_version_info version_info;
};

/**
 *  get_dynamic_global_properties
 */
struct get_dynamic_global_properties_return {
   extended_dynamic_global_properties properties;
};

/**
 *  get_witness_schedule_object
 */
struct get_witness_schedule_object_return {
   api_witness_schedule_object schedule_obj;
};

/**
 *  get_hardfork_property_object
 */
struct get_hardfork_property_object_return {
   api_hardfork_property_object hf_obj;
};


/**
 * @brief Macro defining api method _args and _return types
 */
#define DEFINE_API_ARGS( api_name, arg_type, return_type )  \
typedef arg_type api_name ## _args;                         \
typedef return_type api_name ## _return;


DEFINE_API_ARGS( info,			                                    json_rpc::void_type,			                           info_return);
DEFINE_API_ARGS( about,			                                    json_rpc::void_type,			                           about_return);
DEFINE_API_ARGS( get_block,			                              get_block_args,			                              get_block_return);
DEFINE_API_ARGS( get_ops_in_block,			                        get_ops_in_block_args,			                        get_ops_in_block_return);
DEFINE_API_ARGS( get_feed_history,			                        get_feed_history_args,			                        get_feed_history_return);
DEFINE_API_ARGS( get_active_witnesses,			                     json_rpc::void_type,			                           get_active_witnesses_return);
DEFINE_API_ARGS( get_account,			                              get_account_args,			                              get_account_return);
DEFINE_API_ARGS( get_accounts,			                           get_accounts_args,			                           get_accounts_return);
DEFINE_API_ARGS( get_transaction,			                        get_transaction_args,			                        get_transaction_return);
DEFINE_API_ARGS( create_account,			                           create_account_args,			                           create_account_return);
DEFINE_API_ARGS( update_account,			                           update_account_args,			                           update_account_return);
DEFINE_API_ARGS( update_account_auth,			                     update_account_auth_args,			                     update_account_auth_return);
DEFINE_API_ARGS( delete_account,			                           delete_account_args,			                           delete_account_return);
DEFINE_API_ARGS( get_transaction_id,			                     get_transaction_id_args,			                     get_transaction_id_return);
DEFINE_API_ARGS( list_witnesses,			                           list_witnesses_args,			                           list_witnesses_return);
DEFINE_API_ARGS( list_witnesses_by_vote,			                  list_witnesses_by_vote_args,			                  list_witnesses_by_vote_return);
DEFINE_API_ARGS( get_witness,			                              get_witness_args,			                              get_witness_return);
DEFINE_API_ARGS( update_witness,			                           update_witness_args,			                           update_witness_return);
DEFINE_API_ARGS( stop_witness,			                           stop_witness_args,			                           stop_witness_return);
DEFINE_API_ARGS( set_voting_proxy,			                        set_voting_proxy_args,			                        set_voting_proxy_return);
DEFINE_API_ARGS( vote_for_witness,			                        vote_for_witness_args,			                        vote_for_witness_return);
DEFINE_API_ARGS( transfer,			                                 transfer_args,			                                 transfer_return);
DEFINE_API_ARGS( transfer_to_vesting,			                     transfer_to_vesting_args,			                     transfer_to_vesting_return);
DEFINE_API_ARGS( withdraw_vesting,			                        withdraw_vesting_args,			                        withdraw_vesting_return);
DEFINE_API_ARGS( get_owner_history,			                        get_owner_history_args,			                        get_owner_history_return);
DEFINE_API_ARGS( create_application,			                     create_application_args,			                     create_application_return);
DEFINE_API_ARGS( update_application,			                     update_application_args,			                     update_application_return);
DEFINE_API_ARGS( delete_application,			                     delete_application_args,			                     delete_application_return);
DEFINE_API_ARGS( buy_application,			                        buy_application_args,			                        buy_application_return);
DEFINE_API_ARGS( cancel_application_buying,			               cancel_application_buying_args,			               cancel_application_buying_return);
DEFINE_API_ARGS( get_application_buyings,			                  get_application_buyings_args,			                  get_application_buyings_return);
DEFINE_API_ARGS( make_custom_json_operation,			               make_custom_json_operation_args,			               make_custom_json_operation_return);
DEFINE_API_ARGS( make_custom_binary_operation,			            make_custom_binary_operation_args,			            make_custom_binary_operation_return);
DEFINE_API_ARGS( broadcast_transaction,			                  broadcast_transaction_args,			                  broadcast_transaction_return);
DEFINE_API_ARGS( create_transaction,			                     create_transaction_args,			                     create_transaction_return);
DEFINE_API_ARGS( create_simple_transaction,			               create_simple_transaction_args,			               create_simple_transaction_return);
DEFINE_API_ARGS( get_applications,			                        get_applications_args,			                        get_applications_return);
DEFINE_API_ARGS( get_applications_by_ids,			                  get_applications_by_ids_args,			                  get_applications_by_ids_return);
DEFINE_API_ARGS( get_transaction_digest,			                  get_transaction_digest_args,			                  get_transaction_digest_return);
DEFINE_API_ARGS( add_signature,			                           add_signature_args,			                           add_signature_return);
DEFINE_API_ARGS( add_fee,			                                 add_fee_args,			                                 add_fee_return);
DEFINE_API_ARGS( sign_digest,			                              sign_digest_args,			                              sign_digest_return);
DEFINE_API_ARGS( send_and_sign_operation,			                  send_and_sign_operation_args,			                  send_and_sign_operation_return);
DEFINE_API_ARGS( send_and_sign_transaction,			               send_and_sign_transaction_args,			               send_and_sign_transaction_return);
DEFINE_API_ARGS( verify_signature,			                        verify_signature_args,			                        verify_signature_return);
DEFINE_API_ARGS( generate_key_pair,			                        json_rpc::void_type,			                           generate_key_pair_return);
DEFINE_API_ARGS( generate_key_pair_from_brain_key,			         generate_key_pair_from_brain_key_args,			         generate_key_pair_from_brain_key_return);
DEFINE_API_ARGS( get_public_key,			                           get_public_key_args,			                           get_public_key_return);
DEFINE_API_ARGS( from_base64,			                              from_base64_args,			                              from_base64_return);
DEFINE_API_ARGS( to_base64,			                              to_base64_args,			                              to_base64_return);
DEFINE_API_ARGS( encrypt_data,			                           encrypt_data_args,			                           encrypt_data_return);
DEFINE_API_ARGS( decrypt_data,			                           decrypt_data_args,			                           decrypt_data_return);
DEFINE_API_ARGS( account_exist,			                           account_exist_args,			                           account_exist_return);
DEFINE_API_ARGS( get_account_history,			                     get_account_history_args,			                     get_account_history_return);
DEFINE_API_ARGS( get_received_documents,			                  get_received_documents_args,			                  get_received_documents_return);
DEFINE_API_ARGS( get_account_history,			                     get_account_history_args,			                     get_account_history_return);
DEFINE_API_ARGS( get_received_documents,			                  get_received_documents_args,			                  get_received_documents_return);
DEFINE_API_ARGS( get_active_authority,			                     get_active_authority_args,			                     get_active_authority_return);
DEFINE_API_ARGS( get_owner_authority,			                     get_owner_authority_args,			                     get_owner_authority_return);
DEFINE_API_ARGS( get_memo_key,			                           get_memo_key_args,			                           get_memo_key_return);
DEFINE_API_ARGS( get_account_balance,			                     get_account_balance_args,			                     get_account_balance_return);
DEFINE_API_ARGS( get_vesting_balance,			                     get_vesting_balance_args,			                     get_vesting_balance_return);
DEFINE_API_ARGS( create_simple_authority,			                  create_simple_authority_args,			                  create_simple_authority_return);
DEFINE_API_ARGS( create_simple_multisig_authority,			         create_simple_multisig_authority_args,			         create_simple_multisig_authority_return);
DEFINE_API_ARGS( create_simple_managed_authority,			         create_simple_managed_authority_args,			         create_simple_managed_authority_return);
DEFINE_API_ARGS( create_simple_multisig_managed_authority,			create_simple_multisig_managed_authority_args,			create_simple_multisig_managed_authority_return);
DEFINE_API_ARGS( get_account_name_from_seed,			               get_account_name_from_seed_args,			               get_account_name_from_seed_return);
DEFINE_API_ARGS( get_required_signatures,			                  get_required_signatures_args,			                  get_required_signatures_return);
DEFINE_API_ARGS( calculate_fee,			                           calculate_fee_args,			                           calculate_fee_return);
DEFINE_API_ARGS( fiat_to_sphtx,			                           fiat_to_sphtx_args,			                           fiat_to_sphtx_return);
DEFINE_API_ARGS( custom_object_subscription,			               custom_object_subscription_args,			               custom_object_subscription_return);
DEFINE_API_ARGS( sponsor_account_fees,			                     sponsor_account_fees_args,			                     sponsor_account_fees_return);
DEFINE_API_ARGS( get_version,			                              json_rpc::void_type,			                           get_version_return);
DEFINE_API_ARGS( get_dynamic_global_properties,			            json_rpc::void_type,			                           get_dynamic_global_properties_return);
DEFINE_API_ARGS( get_key_references,			                     get_key_references_args,			                     get_key_references_return);
DEFINE_API_ARGS( get_witness_schedule_object,			            json_rpc::void_type,			                           get_witness_schedule_object_return);
DEFINE_API_ARGS( get_hardfork_property_object,			            json_rpc::void_type,			                           get_hardfork_property_object_return);

#undef DEFINE_API_ARGS

} } } // sophiatx::plugins::alexandria_api



/**
 * info
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::info_return,
			(info) )

/**
 * about
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::about_return,
			(about) )

/**
 * get_block
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_block_args,
			(num) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_block_return,
			(block) )

/**
 * get_ops_in_block
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_ops_in_block_args,
			(block_num)(only_virtual) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_ops_in_block_return,
			(ops_in_block) )

/**
 * get_feed_history
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_feed_history_args,
			(symbol) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_feed_history_return,
			(feed_history) )

/**
 * get_active_witnesses
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_active_witnesses_return,
			(active_witnesses) )

/**
 * get_account
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_return,
			(account) )

/**
 * get_accounts
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_accounts_args,
            (account_names) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_accounts_return,
            (accounts) )

/**
 * get_transaction
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_transaction_args,
			(tx_id) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_transaction_return,
			(tx) )


/**
 * create_account
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_account_args,
			(creator)(name_seed)(json_meta)(owner)(active)(memo) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_account_return,
			(op) )

/**
 * update_account
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::update_account_args,
			(account_name)(json_meta)(owner)(active)(memo) )
FC_REFLECT( sophiatx::plugins::alexandria_api::update_account_return,
			(op) )

/**
 * update_account_auth
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::update_account_auth_args,
            (account_name)(type)(new_authority) )
FC_REFLECT( sophiatx::plugins::alexandria_api::update_account_auth_return,
            (op) )


/**
 * delete_account
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::delete_account_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::delete_account_return,
			(op) )

/**
 * get_transaction_id
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_transaction_id_args,
			(tx) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_transaction_id_return,
			(tx_id) )

/**
 * list_witnesses
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_args,
			(start)(limit) )
FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_return,
			(witnesses) )

/**
 * list_witnesses_by_vote
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_by_vote_args,
			(name)(limit) )
FC_REFLECT( sophiatx::plugins::alexandria_api::list_witnesses_by_vote_return,
			(witnesses_by_vote) )

/**
 * get_witness
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_witness_args,
			(owner_account) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_witness_return,
			(witness) )

/**
 * update_witness
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::update_witness_args,
			(witness_account_name)(url)(block_signing_key)(props) )
FC_REFLECT( sophiatx::plugins::alexandria_api::update_witness_return,
			(op) )

/**
 * stop_witness
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::stop_witness_args,
			(witness_account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::stop_witness_return,
			(op) )

/**
 * set_voting_proxy
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::set_voting_proxy_args,
			(account_to_modify)(proxy) )
FC_REFLECT( sophiatx::plugins::alexandria_api::set_voting_proxy_return,
			(op) )

/**
 * vote_for_witness
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::vote_for_witness_args,
			(voting_account)(witness_to_vote_for)(approve) )
FC_REFLECT( sophiatx::plugins::alexandria_api::vote_for_witness_return,
			(op) )

/**
 * transfer
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::transfer_args,
			(from)(to)(amount)(memo) )
FC_REFLECT( sophiatx::plugins::alexandria_api::transfer_return,
			(op) )

/**
 * transfer_to_vesting
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::transfer_to_vesting_args,
			(from)(to)(amount) )
FC_REFLECT( sophiatx::plugins::alexandria_api::transfer_to_vesting_return,
			(op) )

/**
 * withdraw_vesting
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::withdraw_vesting_args,
			(from)(vesting_shares) )
FC_REFLECT( sophiatx::plugins::alexandria_api::withdraw_vesting_return,
			(op) )

/**
 * get_owner_history
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_owner_history_args,
			(account) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_owner_history_return,
			(owner_history) )

/**
 * create_application
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_application_args,
			(author)(app_name)(url)(meta_data)(price_param) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_application_return,
			(op) )

/**
 * update_application
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::update_application_args,
			(author)(app_name)(new_author)(url)(meta_data)(price_param) )
FC_REFLECT( sophiatx::plugins::alexandria_api::update_application_return,
			(op) )

/**
 * delete_application
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::delete_application_args,
			(author)(app_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::delete_application_return,
			(op) )

/**
 * buy_application
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::buy_application_args,
			(buyer)(app_id) )
FC_REFLECT( sophiatx::plugins::alexandria_api::buy_application_return,
			(op) )

/**
 * cancel_application_buying
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::cancel_application_buying_args,
			(app_owner)(buyer)(app_id) )
FC_REFLECT( sophiatx::plugins::alexandria_api::cancel_application_buying_return,
			(op) )

/**
 * get_application_buyings
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_application_buyings_args,
			(name)(search_type)(count) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_application_buyings_return,
			(application_buyings) )

/**
 * make_custom_json_operation
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::make_custom_json_operation_args,
			(app_id)(from)(to)(json) )
FC_REFLECT( sophiatx::plugins::alexandria_api::make_custom_json_operation_return,
			(op) )

/**
 * make_custom_binary_operation
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::make_custom_binary_operation_args,
			(app_id)(from)(to)(data) )
FC_REFLECT( sophiatx::plugins::alexandria_api::make_custom_binary_operation_return,
			(op) )

/**
 * broadcast_transaction
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::broadcast_transaction_args,
			(tx) )
FC_REFLECT( sophiatx::plugins::alexandria_api::broadcast_transaction_return,
			(tx) )

/**
 * create_transaction
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_transaction_args,
			(op_vec) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_transaction_return,
			(tx) )

/**
 * create_simple_transaction
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_transaction_args,
			(op) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_transaction_return,
			(simple_tx) )

/**
 * get_applications
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_applications_args,
			(names) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_applications_return,
			(applications) )

/**
 * get_applications_by_ids
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_applications_by_ids_args,
			(ids) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_applications_by_ids_return,
			(applications) )

/**
 * get_transaction_digest
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_transaction_digest_args,
			(tx) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_transaction_digest_return,
			(tx_digest) )

/**
 * add_signature
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::add_signature_args,
			(tx)(signature) )
FC_REFLECT( sophiatx::plugins::alexandria_api::add_signature_return,
			(signed_tx) )

/**
 * add_fee
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::add_fee_args,
			(op)(fee) )
FC_REFLECT( sophiatx::plugins::alexandria_api::add_fee_return,
			(op) )

/**
 * sign_digest
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::sign_digest_args,
			(digest)(pk) )
FC_REFLECT( sophiatx::plugins::alexandria_api::sign_digest_return,
			(signed_digest) )

/**
 * send_and_sign_operation
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::send_and_sign_operation_args,
			(op)(pk) )
FC_REFLECT( sophiatx::plugins::alexandria_api::send_and_sign_operation_return,
			(signed_tx) )

/**
 * send_and_sign_transaction
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::send_and_sign_transaction_args,
			(tx)(pk) )
FC_REFLECT( sophiatx::plugins::alexandria_api::send_and_sign_transaction_return,
			(signed_tx) )

/**
 * verify_signature
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::verify_signature_args,
			(digest)(pub_key)(signature) )
FC_REFLECT( sophiatx::plugins::alexandria_api::verify_signature_return,
			(signature_valid) )

/**
 * generate_key_pair
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::generate_key_pair_return,
			(key_pair) )

/**
 * generate_key_pair_from_brain_key
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::generate_key_pair_from_brain_key_args,
			(brain_key) )
FC_REFLECT( sophiatx::plugins::alexandria_api::generate_key_pair_from_brain_key_return,
			(key_pair) )

/**
 * get_public_key
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_public_key_args,
			(private_key) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_public_key_return,
			(public_key) )

/**
 * from_base64
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::from_base64_args,
			(data) )
FC_REFLECT( sophiatx::plugins::alexandria_api::from_base64_return,
			(str) )

/**
 * to_base64
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::to_base64_args,
			(data) )
FC_REFLECT( sophiatx::plugins::alexandria_api::to_base64_return,
			(base64_str) )

/**
 * encrypt_data
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::encrypt_data_args,
			(data)(public_key)(private_key) )
FC_REFLECT( sophiatx::plugins::alexandria_api::encrypt_data_return,
			(encrypted_data) )

/**
 * decrypt_data
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::decrypt_data_args,
			(data)(public_key)(private_key) )
FC_REFLECT( sophiatx::plugins::alexandria_api::decrypt_data_return,
			(decrypted_data) )

/**
 * account_exist
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::account_exist_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::account_exist_return,
			(account_exist) )

/**
 * get_account_history
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_history_args,
			(account)(start)(limit) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_history_return,
			(account_history) )

/**
 * get_received_documents
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_received_documents_args,
			(app_id)(account_name)(search_type)(start)(count) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_received_documents_return,
			(received_documents) )


/**
 * get_active_authority
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_active_authority_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_active_authority_return,
			(active_authority) )

/**
 * get_owner_authority
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_owner_authority_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_owner_authority_return,
			(owner_authority) )

/**
 * get_memo_key
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_memo_key_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_memo_key_return,
			(memo_key) )

/**
 * get_account_balance
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_balance_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_balance_return,
			(account_balance) )

/**
 * get_vesting_balance
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_vesting_balance_args,
			(account_name) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_vesting_balance_return,
			(vesting_balance) )

/**
 * create_simple_authority
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_authority_args,
			(pub_key) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_authority_return,
			(simple_authority) )

/**
 * create_simple_multisig_authority
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_multisig_authority_args,
			(pub_keys)(required_signatures) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_multisig_authority_return,
			(simple_multisig_authority) )

/**
 * create_simple_managed_authority
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_managed_authority_args,
			(managing_account) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_managed_authority_return,
			(simple_managed_authority) )

/**
 * create_simple_multisig_managed_authority
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_multisig_managed_authority_args,
			(managing_accounts)(required_signatures) )
FC_REFLECT( sophiatx::plugins::alexandria_api::create_simple_multisig_managed_authority_return,
			(simple_multisig_managed_authority) )

/**
 * get_account_name_from_seed
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_name_from_seed_args,
			(seed) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_account_name_from_seed_return,
			(account_name) )

/**
 * get_required_signatures
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_required_signatures_args,
			(tx) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_required_signatures_return,
			(required_signatures) )

/**
 * calculate_fee
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::calculate_fee_args,
			(op)(symbol) )
FC_REFLECT( sophiatx::plugins::alexandria_api::calculate_fee_return,
			(fee) )

/**
 * fiat_to_sphtx
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::fiat_to_sphtx_args,
			(fiat) )
FC_REFLECT( sophiatx::plugins::alexandria_api::fiat_to_sphtx_return,
			(sphtx) )

/**
 * custom_object_subscription
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::custom_object_subscription_args,
			(return_id)(app_id)(account_name)(search_type)(start) )
FC_REFLECT( sophiatx::plugins::alexandria_api::custom_object_subscription_return,
			(subscription) )

/**
 * sponsor_account_fees
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::sponsor_account_fees_args,
			(sponsoring_account)(sponsored_account)(is_sponsoring) )
FC_REFLECT( sophiatx::plugins::alexandria_api::sponsor_account_fees_return,
			(op) )

/**
 * get_version
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_version_return,
         (version_info) )

/**
 * get_dynamic_global_properties
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_dynamic_global_properties_return,
         (properties) )

/**
 * get_key_references
 */
FC_REFLECT( sophiatx::plugins::alexandria_api::get_key_references_args,
            (keys) )
FC_REFLECT( sophiatx::plugins::alexandria_api::get_key_references_return,
            (accounts) )

/**
* get_witness_schedule_object
*/
FC_REFLECT( sophiatx::plugins::alexandria_api::get_witness_schedule_object_return,
            (schedule_obj) )

/**
* get_hardfork_property_object
*/
FC_REFLECT( sophiatx::plugins::alexandria_api::get_hardfork_property_object_return,
            (hf_obj) )