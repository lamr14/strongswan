/**
 * @file child_proposal.h
 * 
 * @brief Interface of child_proposal_t.
 *
 */

/*
 * Copyright (C) 2006 Martin Willi
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef _CHILD_PROPOSAL_H_
#define _CHILD_PROPOSAL_H_

#include <types.h>
#include <utils/identification.h>
#include <utils/linked_list.h>
#include <network/host.h>
#include <transforms/crypters/crypter.h>
#include <transforms/signers/signer.h>
#include <transforms/diffie_hellman.h>
#include <config/traffic_selector.h>


typedef enum protocol_id_t protocol_id_t;

/**
 * Protocol ID of a proposal.
 * 
 * @ingroup config
 */
enum protocol_id_t {
	UNDEFINED_PROTOCOL_ID = 201,
	IKE = 1,
	AH = 2,
	ESP = 3,
};

/** 
 * String mappings for protocol_id_t.
 * 
 * @ingroup config
 */
extern mapping_t protocol_id_m[];


typedef enum transform_type_t transform_type_t;

/**
 * Type of a transform, as in IKEv2 draft 3.3.2.
 * 
 * @ingroup payloads
 */
enum transform_type_t {
	UNDEFINED_TRANSFORM_TYPE = 241,
	ENCRYPTION_ALGORITHM = 1,
	PSEUDO_RANDOM_FUNCTION = 2,
	INTEGRITY_ALGORITHM = 3,
	DIFFIE_HELLMAN_GROUP = 4,
	EXTENDED_SEQUENCE_NUMBERS = 5
};

/** 
 * String mappings for transform_type_t.
 * 
 * @ingroup payloads
 */
extern mapping_t transform_type_m[];


typedef enum extended_sequence_numbers_t extended_sequence_numbers_t;

/** 
 * Extended sequence numbers, as in IKEv2 draft 3.3.2.
 * 
 * @ingroup payloads
 */
enum extended_sequence_numbers_t {
	NO_EXT_SEQ_NUMBERS = 0,
	EXT_SEQ_NUMBERS = 1
};

/** 
 * String mappings for extended_sequence_numbers_t.
 * 
 * @ingroup payloads
 */
extern mapping_t extended_sequence_numbers_m[];


typedef struct algorithm_t algorithm_t;

/**
 * Struct used to store different kinds of algorithms. The internal
 * lists of algorithms contain such structures.
 */
struct algorithm_t {
	/**
	 * Value from an encryption_algorithm_t/integrity_algorithm_t/...
	 */
	u_int16_t algorithm;
	
	/**
	 * the associated key size, or zero if not needed
	 */
	u_int16_t key_size;
};

typedef struct child_proposal_t child_proposal_t;

/**
 * @brief Stores a proposal for a child SA.
 * 
 * A child_proposal may contain more than one algorithm
 * of the same kind. ONE of them can be selected.
 *
 * @warning This class is NOT thread-save!
 * 
 * @b Constructors:
 *   - child_proposal_create()
 * 
 * @ingroup config
 */
struct child_proposal_t {
	
	/**
	 * @brief Add an algorithm to the proposal.
	 * 
	 * The algorithms are stored by priority, first added
	 * is the most preferred.
	 * Key size is only needed for encryption algorithms
	 * with variable key size (such as AES), or integrity
	 * algorithms.
	 * The alg parameter accepts encryption_algorithm_t,
	 * integrity_algorithm_t, dh_group_number_t and
	 * extended_sequence_numbers_t.
	 * 
	 * @warning Do not add while other threads are reading.
	 * 
	 * @param this					calling object
	 * @param proto					desired protocol
	 * @param type					kind of algorithm
	 * @param alg					identifier for algorithm
	 * @param key_size				key size to use
	 */
	void (*add_algorithm) (child_proposal_t *this, protocol_id_t proto, transform_type_t type, u_int16_t alg, size_t key_size);
	
	/**
	 * @brief Get an iterator over algorithms for a specifc protocol/algo type.
	 * 
	 * @param this					calling object
	 * @param proto					desired protocol
	 * @param type					kind of algorithm
	 * @return						iterator over algorithms
	 */
	iterator_t *(*create_algorithm_iterator) (child_proposal_t *this, protocol_id_t proto, transform_type_t type);
	
	/**
	 * @brief Get the algorithm for a type to use.
	 * 
	 * If there are multiple algorithms, only the first is returned.
	 * Result is still owned by child_proposal, do not modify!
	 * 
	 * @param this					calling object
	 * @param proto					desired protocol
	 * @param type					kind of algorithm
	 * @param[out] algo				pointer which receives algorithm and key size
	 * @return						TRUE if algorithm of this kind available
	 */
	bool (*get_algorithm) (child_proposal_t *this, protocol_id_t proto, transform_type_t type, algorithm_t** algo);

	/**
	 * @brief Compare two proposal, and select a matching subset.
	 * 
	 * If the proposals are for the same protocols (AH/ESP), they are
	 * compared. If they have at least one algorithm of each type
	 * in common, a resulting proposal of this kind is created.
	 * 
	 * @param this					calling object
	 * @param other					proposal to compair agains
	 * @return						
	 * 								- selected proposal, if possible
	 * 								- NULL, if proposals don't match
	 */
	child_proposal_t *(*select) (child_proposal_t *this, child_proposal_t *other);
	
	/**
	 * @brief Get the number set on construction.
	 * 
	 * @param this				calling object
	 * @return 					number
	 */
	u_int8_t (*get_number) (child_proposal_t *this);
	
	/**
	 * @brief Get the protocol ids in the proposals.
	 * 
	 * With AH and ESP, there could be two protocols in one
	 * proposal.
	 * 
	 * @param this				calling object
	 * @param ids 				array of protocol ids, 
	 */
	void (*get_protocols) (child_proposal_t *this, protocol_id_t ids[2]);
	
	/**
	 * @brief Get the spi for a specific protocol.
	 * 
	 * @param this				calling object
	 * @param proto				AH/ESP
	 * @return					spi for proto
	 */
	u_int64_t (*get_spi) (child_proposal_t *this, protocol_id_t proto);
	
	/**
	 * @brief Set the spi for a specific protocol.
	 * 
	 * @param this				calling object
	 * @param proto 			AH/ESP
	 * @param spi				spi to set for proto
	 */
	void (*set_spi) (child_proposal_t *this, protocol_id_t proto, u_int64_t spi);
	
	/**
	 * @brief Destroys the proposal object.
	 * 
	 * @param this				calling object
	 */
	void (*destroy) (child_proposal_t *this);
};

/**
 * @brief Create a child proposal for AH and/or ESP.
 * 
 * @param number			number of the proposal, as in the payload
 * @return 					child_proposal_t object
 * 
 * @ingroup config
 */
child_proposal_t *child_proposal_create(u_int8_t number);

#endif //_CHILD_PROPOSAL_H_
