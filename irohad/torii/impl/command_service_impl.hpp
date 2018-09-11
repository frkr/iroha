/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORII_COMMAND_SERVICE_IMPL_HPP
#define TORII_COMMAND_SERVICE_IMPL_HPP

#include "torii/command_service.hpp"

#include "endpoint.pb.h"

#include "ametsuchi/storage.hpp"
#include "backend/protobuf/transaction_responses/proto_tx_response.hpp"
#include "cache/cache.hpp"
#include "cryptography/hash.hpp"
#include "logger/logger.hpp"
#include "torii/processor/transaction_processor.hpp"
#include "torii/status_bus.hpp"

namespace torii {
  /**
   * Actual implementation of sync CommandServiceImpl.
   */
  class CommandServiceImpl : public CommandService {
   public:
    /**
     * Creates a new instance of CommandServiceImpl
     * @param tx_processor - processor of received transactions
     * @param storage - to query transactions outside the cache
     * @param status_bus is a common notifier for tx statuses
     */
    CommandServiceImpl(
        std::shared_ptr<iroha::torii::TransactionProcessor> tx_processor,
        std::shared_ptr<iroha::ametsuchi::Storage> storage,
        std::shared_ptr<iroha::torii::StatusBus> status_bus);

    /**
     * Disable copying in any way to prevent potential issues with common
     * storage/tx_processor
     */
    CommandServiceImpl(const CommandServiceImpl &) = delete;
    CommandServiceImpl &operator=(const CommandServiceImpl &) = delete;

    void handleTransactionList(
        const shared_model::interface::TransactionSequence &tx_list) override;
    void handleTransactionListError(
        const std::vector<shared_model::crypto::Hash> &tx_hashes,
        const std::string &error) override;

    std::shared_ptr<shared_model::interface::TransactionResponse> getStatus(
        const shared_model::crypto::Hash &request) override;
    rxcpp::observable<
        std::shared_ptr<shared_model::interface::TransactionResponse>>
    getStatusStream(const shared_model::crypto::Hash &hash) override;

   private:
    /**
     * Execute events scheduled in run loop until it is not empty and the
     * subscriber is active
     * @param subscription - tx status subscription
     * @param run_loop - gRPC thread run loop
     */
    inline void handleEvents(rxcpp::composite_subscription &subscription,
                             rxcpp::schedulers::run_loop &run_loop);

    /**
     * Share tx status and log it
     * @param who identifier for the logging
     * @param response to be shared
     */
    void pushStatus(
        const std::string &who,
        std::shared_ptr<shared_model::interface::TransactionResponse> response);

    /**
     * Forward batch to transaction processor and set statuses of all
     * transactions inside it
     * @param batch to be processed
     */
    void processBatch(const shared_model::interface::TransactionBatch &batch);

   private:
    using CacheType = iroha::cache::Cache<
        shared_model::crypto::Hash,
        std::shared_ptr<shared_model::interface::TransactionResponse>,
        shared_model::crypto::Hash::Hasher>;

    std::shared_ptr<iroha::torii::TransactionProcessor> tx_processor_;
    std::shared_ptr<iroha::ametsuchi::Storage> storage_;
    std::shared_ptr<iroha::torii::StatusBus> status_bus_;
    std::shared_ptr<CacheType> cache_;

    logger::Logger log_;
  };

}  // namespace torii

#endif  // TORII_COMMAND_SERVICE_IMPL_HPP
