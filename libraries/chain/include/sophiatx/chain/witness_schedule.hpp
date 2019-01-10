#pragma once

namespace sophiatx { namespace chain {

class database_interface;

void update_witness_schedule( const std::shared_ptr<database_interface>& db );
void reset_virtual_schedule_time( const std::shared_ptr<database_interface>& db );

} }
