//
// Created by rbcheng on 18-11-27.
// Email: rbcheng@qq.com
//

#ifndef ECHO_CONN_POOL_H
#define ECHO_CONN_POOL_H

#include <unordered_map>
#include "handy.h"

namespace handy {

    enum HandyProtocol {TCP, HTTP};

    struct ConnectionPack {
        HandyProtocol proto;
        TcpConnPtr conn;
    };

    class TcpConnPool {
    private:
        EventBase* base = nullptr;
        std::unordered_map<__int64_t , ConnectionPack> pool;
        TcpConnPool() {}

        TimerId timer_id;

    public:
        static TcpConnPool& get_pool() {
            static TcpConnPool pool;
            return pool;
        }

        /**
         * Register a default state callback to {@param con}
         * @param proto : the protocol of connection
         * @param con : the tcp connection
         */
        void register_state_cb(const HandyProtocol& proto, const TcpConnPtr& con) {
            con->onState([=](const TcpConnPtr& con) {
                if (con->getState() == TcpConn::State::Connected) {
                    info("connected, id: %ld", con->getChannel()->id());
                    register_con(proto, con);
                }

                if (con->getState() == TcpConn::State::Closed) {
                    info("closed, id: %ld", con->getChannel()->id());
                    unregister_con(con->getChannel()->id());
                }
            });
        }



        /**
         * Register a default state callback to {@param con}, the protocol will be set to TCP in default.
         * @param con: the tcp connection
         */
        void register_state_cb(const TcpConnPtr& con) {
            register_state_cb(TCP, con);
        }

        // TODO not finish
        void start_schedule(EventBase* base) {
            if (this->base == nullptr) {
                this->base = base->allocBase();
            } else {
                // cancel original task
                this->base->cancel(this->timer_id);
            }

            this->timer_id = base->runAfter(1000, [&](){
                for (auto& item: pool) {
                    info("timer id: %ld, con state: %d ", item.first, item.second.conn->getState());
                    if (item.second.conn->getState() != TcpConn::State::Connected) {
                        unregister_con(item.second.conn->getChannel()->id());
                    }
                }
            }, 1000);

        }

        // TODO not finish
        void stop_schedule() {
            base->cancel(this->timer_id);
            this->base = nullptr;
        }

        /**
         * Register a connection to {@param pool}, the protocol will be set to TCP in default.
         * @param con : the tcp connection
         * @return
         */
        bool register_con(const TcpConnPtr& con) {
            return register_con(TCP, con);
        }

        /**
         * Register a connection to {@param pool}
         * @param proto : the protocol of connection
         * @param con : the tcp connection
         * @return
         */
        bool register_con(const HandyProtocol& proto, const TcpConnPtr& con) {
            __int64_t con_id = con->getChannel()->id();
            ConnectionPack pack = {proto, con};
            if (!exist(con_id)) {
                pool[con_id] = pack;
                return true;
            } else {

                return false;
            }
        }

        /**
         * Unregister a connection from {@param pool}
         * @param con_id : the id of connection
         * @return
         */
        bool unregister_con(__int64_t con_id) {
            if (exist(con_id)) {
                pool.erase(con_id);
                return true;
            } else {
                return false;
            }
        }

        /**
         * Get TcpConnPtr according to the id of tcp connection.
         * @param con_id : the id of tcp connection
         * @return
         */
        TcpConnPtr get_con(const __int64_t con_id) {
            return exist(con_id) ? pool[con_id].conn : nullptr;
        }

        /**
         * Get ConnectionPack according to the id of tcp connection.
         * @param con_id
         * @return
         */
        ConnectionPack get_con_pack(const __int64_t con_id) {
            return exist(con_id) ? pool[con_id] : ConnectionPack();
        }

        /**
         * Check a connection if exists in {@param pool}
         * @param con_id : the id of tcp connection
         * @return
         */
        bool exist(__int64_t con_id) {
            return pool.find(con_id) != pool.end();
        }

    };

}


#endif //ECHO_CONN_POOL_H
