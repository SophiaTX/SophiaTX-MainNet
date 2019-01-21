#pragma once
#include <sophiatx/protocol/exceptions.hpp>
#include <sophiatx/protocol/operations.hpp>

namespace sophiatx { namespace chain {

class database;

template< typename OperationType=sophiatx::protocol::operation >
class evaluator
{
   public:
      virtual ~evaluator() {}

      virtual void apply(const OperationType& op) = 0;
      virtual int get_type()const = 0;
};

template< typename EvaluatorType, typename OperationType=sophiatx::protocol::operation >
class evaluator_impl : public evaluator<OperationType>
{
   public:
      typedef OperationType operation_sv_type;
      // typedef typename EvaluatorType::operation_type op_type;

      evaluator_impl( const std::shared_ptr<database>& d )
         : _db(d) {}

      virtual ~evaluator_impl() {}

      virtual void apply(const OperationType& o) final override
      {
         auto* eval = static_cast< EvaluatorType* >(this);
         const auto& op = o.template get< typename EvaluatorType::operation_type >();
         eval->do_apply(op);
      }

      virtual int get_type()const override { return OperationType::template tag< typename EvaluatorType::operation_type >::value; }

      std::shared_ptr<database>& db() { return _db; }

   protected:
      std::shared_ptr<database> _db;
};

} }

#define SOPHIATX_DEFINE_EVALUATOR( X ) \
class X ## _evaluator : public sophiatx::chain::evaluator_impl< X ## _evaluator > \
{                                                                           \
   public:                                                                  \
      typedef X ## _operation operation_type;                               \
                                                                            \
      X ## _evaluator( const std::shared_ptr<database>& db )                                       \
         : sophiatx::chain::evaluator_impl< X ## _evaluator >( db )          \
      {}                                                                    \
                                                                            \
      void do_apply( const X ## _operation& o );                            \
};

#define SOPHIATX_DEFINE_PLUGIN_EVALUATOR( PLUGIN, OPERATION, X )               \
class X ## _evaluator : public sophiatx::chain::evaluator_impl< X ## _evaluator, OPERATION > \
{                                                                           \
   public:                                                                  \
      typedef X ## _operation operation_type;                               \
                                                                            \
      X ## _evaluator( std::shared_ptr<sophiatx::chain::database>& db, PLUGIN* plugin )       \
         : sophiatx::chain::evaluator_impl< X ## _evaluator, OPERATION >( db ), \
           _plugin( plugin )                                                \
      {}                                                                    \
                                                                            \
      void do_apply( const X ## _operation& o );                            \
                                                                            \
      PLUGIN* _plugin;                                                      \
};
