// Copyright (c) 2017, Baidu.com, Inc. All Rights Reserved

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

package com.baidu.palo.common;

public class Config extends ConfigBase {

    /*
     * This specifies FE log dir. FE will produces 2 log files:
     * fe.log:      all logs of FE process.
     * fe.warn.log  all WARNING and ERROR log of FE process.
     */
    @ConfField public static String sys_log_dir = System.getenv("PALO_HOME") + "/log";
    @ConfField public static String sys_log_level = "INFO"; // INFO, WARNING, ERROR, FATAL
    /*
     * The roll mode of FE log files.
     * TIME-DAY:    roll every day.
     * TIME-HOUR:   roll every hour.
     * SIZE-MB-nnn: roll by size.
     */
    @ConfField public static String sys_log_roll_mode = "SIZE-MB-1024"; // TIME-DAY， TIME-HOUR， SIZE-MB-nnn
    /*
     * Maximal FE log files to be kept.
     * Doesn't work if roll mode is TIME-*
     */
    @ConfField public static int sys_log_roll_num = 10;

    /*
     * Verbose modules. VERBOSE level is implemented by log4j DEBUG level.
     * eg:
     *      sys_log_verbose_modules = com.baidu.palo.catalog
     *  This will only print verbose log of files in package com.baidu.palo.catalog and all its sub packages.
     */
    @ConfField public static String[] sys_log_verbose_modules = {};

    /*
     * This specifies FE audit log dir.
     * Audit log fe.audit.log contains all SQL queries with related infos such as user, host, cost, status, etc.
     */
    @ConfField public static String audit_log_dir = System.getenv("PALO_HOME") + "/log";
    /*
     * Slow query contains all queries which cost exceed *qe_slow_log_ms*
     */
    @ConfField public static String[] audit_log_modules = {"slow_query", "query"};
    @ConfField public static String audit_log_roll_mode = "TIME-DAY"; // TIME-DAY， TIME-HOUR， SIZE-MB-nnn
    @ConfField public static int audit_log_roll_num = 10; // Doesn't work if roll mode is TIME-*

    /*
     * Labels of finished or cancelled load jobs will be removed after *label_keep_max_second*
     * The removed labels can be reused.
     * Set a short time will lower the FE memory usage.
     * (Because all load jobs' info is kept in memoery before being removed)
     */
    @ConfField public static int label_keep_max_second = 7 * 24 * 3600; // 7 days
    /*
     * Load label cleaner will run every *label_clean_interval_second* to clean the outdated jobs.
     */
    @ConfField public static int label_clean_interval_second = 4 * 3600; // 4 hours
    /*
     * If a load job stay in QUORUM_FINISHED state longer than *quorum_load_job_max_second*,
     * a clone job will be triggered to help finishing this load job.
     */
    @ConfField public static int quorum_load_job_max_second = 24 * 3600; // 1 days

    // Configurations for meta data durability
    /*
     * Palo meta data will be saved here.
     * The storage of this dir is highly recommended as to be:
     * 1. High write performance (SSD)
     * 2. Safe (RAID)
     */
    @ConfField public static String meta_dir = System.getenv("PALO_HOME") + "/palo-meta";
    
    /*
     * temp dir is used to save intermediate results of some process, such as backup and restore process.
     * file in this dir will be cleaned after these process is finished.
     */
    @ConfField public static String tmp_dir = System.getenv("PALO_HOME") + "/temp_dir";
    
    /*
     * Edit log type.
     * BDB: write log to bdbje
     * LOCAL: deprecated.
     */
    @ConfField
    public static String edit_log_type = "BDB";
    /*
     * bdbje port
     */
    @ConfField
    public static int edit_log_port = 9010;
    /*
     * Master FE will save image every *edit_log_roll_num* meta journals.
     */
    @ConfField public static int edit_log_roll_num = 50000;
    /*
     * Non-master FE will stop offering service
     * if meta data delay gap exceeds *meta_delay_toleration_second*
     */
    @ConfField public static int meta_delay_toleration_second = 300;    // 5 min
    /*
     * Master FE sync policy of bdbje.
     * more info, see: http://docs.oracle.com/cd/E17277_02/html/java/com/sleepycat/je/Durability.SyncPolicy.html
     */
    @ConfField public static String master_sync_policy = "WRITE_NO_SYNC"; // SYNC, NO_SYNC, WRITE_NO_SYNC
    /*
     * Follower FE sync policy of bdbje.
     */
    @ConfField public static String replica_sync_policy = "WRITE_NO_SYNC"; // SYNC, NO_SYNC, WRITE_NO_SYNC
    /*
     * Replica ack policy of bdbje.
     * more info, see: http://docs.oracle.com/cd/E17277_02/html/java/com/sleepycat/je/Durability.ReplicaAckPolicy.html
     */
    @ConfField public static String replica_ack_policy = "SIMPLE_MAJORITY"; // ALL, NONE, SIMPLE_MAJORITY

    /*
     * Specified an IP for frontend, instead of the ip get by *InetAddress.getByName*.
     * This can be used when *InetAddress.getByName* get an unexpected IP address.
     * Default is "0.0.0.0", which means not set.
     * CAN NOT set this as a hostname, only IP.
     */
    @ConfField public static String frontend_address = "0.0.0.0";

    /*
     * Declare a selection strategy for those servers have many ips.
     * Note that there should at most one ip match this list.
     * this is a list in semicolon-delimited format, in CIDR notation, e.g. 10.10.10.0/24
     * If no ip match this rule, will choose one randomly.
     */
    @ConfField public static String priority_networks = "";

    /*
     * Kudu is currently not supported.
     */
    @ConfField public static String kudu_master_addresses = "127.0.0.1:8030";
    @ConfField public static int kudu_client_timeout_ms = 500;

    /*
     * If true, FE will reset bdbje replication group(that is, to remove all electable nodes info)
     * and is supposed to start as Master.
     * If all the electable nodes can not start, we can copy the meta data
     * to another node and set this config to true to try to restart the FE.
     */
    @ConfField public static String metadata_failure_recovery = "false";

    /*
     * If true, non-master FE will ignore the meta data delay gap between Master FE and its self,
     * even if the metadata delay gap exceeds *meta_delay_toleration_second*.
     * Non-master FE will still offer read service.
     *
     * This is helpful when you try to stop the Master FE for a relatively long time for some reason,
     * but still wish the non-master FE can offer read service.
     */
    @ConfField public static boolean ignore_meta_check = false;

    /*
     * Set the maximum acceptable clock skew between non-master FE to Master FE host.
     * This value is checked whenever a non-master FE establishes a connection to master FE via BDBJE.
     * The connection is abandoned if the clock skew is larger than this value.
     */
    @ConfField public static long max_bdbje_clock_delta_ms = 5000; // 5s

    /*
     * Fe http port
     * Currently, all FEs' http port must be same.
     */
    @ConfField public static int http_port = 8030;
    /*
     * FE thrift server port
     */
    @ConfField public static int rpc_port = 9020;
    /*
     * FE mysql server port
     */
    @ConfField public static int query_port = 9030;

    /*
     * Cluster name will be shown as the title of web page
     */
    @ConfField public static String cluster_name = "Baidu Palo";
    /*
     * node(FE or BE) will be considered belonging to the same Palo cluster if they have same cluster id.
     * Cluster id is usually a random integer generated when master FE start at first time.
     * You can also sepecify one.
     */
    @ConfField public static int cluster_id = -1;
    /*
     * Cluster token used for internal authentication.
     */
    @ConfField public static String auth_token = "";

    // Configurations for load, clone, create table, alter table etc. We will rarely change them
    /*
     * Maximal waiting time for creating a single replica.
     * eg.
     *      if you create a table with #m tablets and #n replicas for each tablet,
     *      the create table request will run at most (m * n * tablet_create_timeout_second) before timeout.
     */
    @ConfField public static int tablet_create_timeout_second = 1;
    
    /*
     * Maximal memory layout length of a row. default is 100 KB.
     * In BE, the maximal size of a RowBlock is 100MB(Configure as max_unpacked_row_block_size in be.conf).
     * And each RowBlock contains 1024 rows. So the maximal size of a row is approximately 100 KB.
     * 
     * eg.
     *      schema: k1(int), v1(decimal), v2(varchar(2000))
     *      then the memory layout length of a row is: 8(int) + 40(decimal) + 2000(varchar) = 2048 (Bytes)
     *      
     * See memory layout length of all types, run 'help create table' in mysql-client.
     * 
     * If you want to increase this number to support more columns in a row, you also need to increase the 
     * max_unpacked_row_block_size in be.conf. But the performance impact is unknown.
     */
    @ConfField
    public static int max_layout_length_per_row = 100000; // 100k

    /*
     * Load checker's running interval.
     * A load job will transfer its state from PENDING to ETL to LOADING to FINISHED.
     * So a load job will cost at least 3 check intervals to finish.
     */
    @ConfField public static int load_checker_interval_second = 5;

    /*
     * Concurrency of HIGH priority pending load jobs.
     * Load job priority is defined as HIGH or NORMAL.
     * All mini batch load jobs are HIGH priority, other types of load jobs are NORMAL priority.
     * Priority is set to avoid that a slow load job occupies a thread for a long time.
     * This is just a internal optimized scheduling policy.
     * Currently, you can not specified the job priority manually,
     * and do not change this if you know what you are doing.
     */
    @ConfField public static int load_pending_thread_num_high_priority = 3;
    /*
     * Concurrency of NORMAL priority pending load jobs.
     * Do not change this if you know what you are doing.
     */
    @ConfField public static int load_pending_thread_num_normal_priority = 10;
    /*
     * Concurrency of HIGH priority etl load jobs.
     * Do not change this if you know what you are doing.
     */
    @ConfField public static int load_etl_thread_num_high_priority = 3;
    /*
     * Concurrency of NORMAL priority etl load jobs.
     * Do not change this if you know what you are doing.
     */
    @ConfField public static int load_etl_thread_num_normal_priority = 10;
    /*
     * Not available.
     */
    @ConfField public static int load_input_size_limit_gb = 0; // GB, 0 is no limit
    /*
     * Not available.
     */
    @ConfField public static int load_running_job_num_limit = 0; // 0 is no limit
    /*
     * Default pull load timeout
     */
    @ConfField public static int pull_load_task_default_timeout_second = 14400; // 4 hour

    /*
     * Default mini load timeout
     */
    @ConfField public static int mini_load_default_timeout_second = 3600; // 1 hour

    /*
     * Default hadoop load timeout
     */
    @ConfField public static int hadoop_load_default_timeout_second = 86400 * 3; // 3 day

    /*
     * Same meaning as *tablet_create_timeout_second*, but used when delete a tablet.
     */
    @ConfField public static int tablet_delete_timeout_second = 2;
    /*
     * Clone checker's running interval.
     */
    @ConfField public static int clone_checker_interval_second = 300;
    /*
     * Default timeout of a single clone job. Set long enough to fit your replica size.
     * The larger the replica data size is, the more time is will cost to finish clone.
     */
    @ConfField public static int clone_job_timeout_second = 7200; // 2h
    /*
     * Concurrency of LOW priority clone jobs.
     * Concurrency of High priority clone jobs is currently unlimit.
     */
    @ConfField public static int clone_max_job_num = 100;
    /*
     * LOW priority clone job's delay trigger time.
     * A clone job contains a tablet which need to be cloned(recovery or migration).
     * If the priority is LOW, it will be delayed *clone_low_priority_delay_second*
     * after the job creation and then be executed.
     * This is to avoid a large number of clone jobs running at same time only because a host is down for a short time.
     *
     * NOTICE that this config(and *clone_normal_priority_delay_second* as well)
     * will not work if it's smaller then *clone_checker_interval_second*
     */
    @ConfField public static int clone_low_priority_delay_second = 600;
    /*
     * NORMAL priority clone job's delay trigger time.
     */
    @ConfField public static int clone_normal_priority_delay_second = 300;
    /*
     * HIGH priority clone job's delay trigger time.
     */
    @ConfField public static int clone_high_priority_delay_second = 0;
    /*
     * Balance threshold of data size in BE.
     * The balance algorithm is:
     * 1. Calculate the average used capacity(AUC) of the entire cluster. (total data size / total backends num)
     * 2. The high water level is (AUC * (1 + clone_capacity_balance_threshold))
     * 3. The low water level is (AUC * (1 - clone_capacity_balance_threshold))
     * The Clone checker will try to move replica from high water level BE to low water level BE.
     */
    @ConfField public static double clone_capacity_balance_threshold = 0.2;
    /*
     * Balance threshold of num of replicas in Backends.
     */
    @ConfField public static double clone_distribution_balance_threshold = 0.2;
    /*
     * Maximal timeout of ALTER TABEL request. Set long enough to fit your table data size.
     */
    @ConfField public static int alter_table_timeout_second = 86400; // 1day
    /*
     * After ALTER TABEL finished, deletion of the old schema replica is delayed,
     * in case there are still some queries using the old schema replica.
     */
    @ConfField public static int alter_delete_base_delay_second = 600; // 10min
    /*
     * If a backend is down for *max_backend_down_time_second*, a BACKEND_DOWN event will be triggered.
     * Do not set this if you know what you are doing.
     */
    @ConfField public static int max_backend_down_time_second = 3600; // 1h
    /*
     * When create a table(or partition), you can specify its storage media(HDD or SSD).
     * If set to SSD, this specifies the default duration that tablets will stay on SSD.
     * After that, tablets will be moved to HDD automatically.
     * You can set storage cooldown time in CREATE TABLE stmt.
     */
    @ConfField public static long storage_cooldown_second = 30 * 24 * 3600L; // 30 days
    /*
     * After dropping database(table/partition), you can recover it by using RECOVER stmt.
     * And this specifies the maximal data retention time. After time, the data will be deleted permanently.
     */
    @ConfField public static long catalog_trash_expire_second = 86400L; // 1day
    /*
     * Maximal bytes that a single broker scanner will read.
     * Do not set this if you know what you are doing.
     */
    @ConfField public static long min_bytes_per_broker_scanner = 67108864L; // 64MB
    /*
     * Maximal concurrency of broker scanners.
     * Do not set this if you know what you are doing.
     */
    @ConfField public static int max_broker_concurrency = 10;

    /*
     * Export checker's running interval.
     */
    @ConfField public static int export_checker_interval_second = 5;
    /*
     * Concurrency of pending export jobs.
     */
    @ConfField public static int export_pending_thread_num = 5;
    /*
     * Num of thread to handle export jobs.
     */
    @ConfField public static int export_exporting_thread_num = 10;
    /*
     * Limitation of the concurrency of running export jobs.
     * Default is no limit.
     */
    @ConfField public static int export_running_job_num_limit = 0; // 0 is no limit
    /*
     * Default timeout of export jobs.
     */
    @ConfField public static int export_task_default_timeout_second = 24 * 3600;    // 24h
    /*
     * Concurrency of exporting tablets.
     */
    @ConfField public static int export_parallel_tablet_num = 5;
    /*
     * Labels of finished or cancelled export jobs will be removed after *label_keep_max_second*.
     * The removed labels can be reused.
     */
    @ConfField public static int export_keep_max_second = 7 * 24 * 3600; // 7 days

    // Configurations for consistency check
    /*
     * Consistency checker will run from *consistency_check_start_time* to *consistency_check_end_time*.
     * Default is from 23:00 to 04:00
     */
    @ConfField public static String consistency_check_start_time = "23";
    @ConfField public static String consistency_check_end_time = "4";
    /*
     * Default timeout of a single consistency check task. Set long enough to fit your tablet size.
     */
    @ConfField public static long check_consistency_default_timeout_second = 600; // 10 min

    // Configurations for query engine
    /*
     * Maximal number of connections per FE.
     */
    @ConfField public static int qe_max_connection = 1024;
    /*
     * Maximal number of connections per user, per FE.
     */
    @ConfField public static int max_conn_per_user = 100;
    /*
     * Default query timeout.
     */
    @ConfField public static int qe_query_timeout_second = 300;
    /*
     * If the response time of a query exceed this threshold, it will be recored in audit log as slow_query.
     */
    @ConfField public static long qe_slow_log_ms = 5000;
    /*
     * The interval of user resource publishing.
     * User resource contains cgroup configurations of a user.
     */
    @ConfField public static int meta_resource_publish_interval_ms = 60000; // 1m
    /*
     * The default user resource publishing timeout.
     */
    @ConfField public static int meta_publish_timeout_ms = 1000;
    @ConfField public static boolean proxy_auth_enable = false;
    @ConfField public static String proxy_auth_magic_prefix = "x@8";
    /*
     * Limit on the number of expr children of an expr tree.
     * Exceed this limit may cause long analysis time while holding database read lock.
     * Do not set this if you know what you are doing.
     */
    @ConfField public static int expr_children_limit = 10000;
    /*
     * Limit on the depth of an expr tree.
     * Exceed this limit may cause long analysis time while holding db read lock.
     * Do not set this if you know what you are doing.
     */
    @ConfField public static int expr_depth_limit = 3000;

    // Configurations for backup and restore
    /*
     * Plugins' path for BACKUP and RESTORE operations. Currently deprecated.
     */
    @ConfField public static String backup_plugin_path = "/tools/trans_file_tool/trans_files.sh";

    // Configurations for hadoop dpp
    /*
     * The following configurations are not available.
     */
    @ConfField public static String dpp_hadoop_client_path = "/lib/hadoop-client/hadoop/bin/hadoop";
    @ConfField public static long dpp_bytes_per_reduce = 100 * 1024 * 1024L; // 100M
    @ConfField public static String dpp_default_cluster = "palo-dpp";
    @ConfField public static String dpp_default_config_str = ""
            + "{"
            + "hadoop_configs : '"
            + "mapred.job.priority=NORMAL;"
            + "mapred.job.map.capacity=50;"
            + "mapred.job.reduce.capacity=50;"
            + "mapred.hce.replace.streaming=false;"
            + "abaci.long.stored.job=true;"
            + "dce.shuffle.enable=false;"
            + "dfs.client.authserver.force_stop=true;"
            + "dfs.client.auth.method=0"
            + "'}";
    @ConfField public static String dpp_config_str = ""
            + "{palo-dpp : {"
            + "hadoop_palo_path : '/dir',"
            + "hadoop_configs : '"
            + "fs.default.name=hdfs://host:port;"
            + "mapred.job.tracker=host:port;"
            + "hadoop.job.ugi=user,password"
            + "'}"
            + "}";

    // For forward compatibility, will be removed later.
    // check token when download image file.
    @ConfField public static boolean enable_token_check = true;

    /*
     * Set to true if you deploy Palo using thirdparty deploy manager
     * Valid options are:
     *      disable:    no deploy manager
     *      k8s:        Kubernetes
     *      ambari:     Ambari
     *      local:      Local File (for test or Boxer2 BCC version)
     */
    @ConfField public static String enable_deploy_manager = "disable";
    
    // If use k8s deploy manager locally, set this to true and prepare the certs files
    @ConfField public static boolean with_k8s_certs = false;
    
    // white list limit
    @ConfField public static int per_user_white_list_limit = 1024;
    
    // Set runtime locale when exec some cmds
    @ConfField public static String locale = "zh_CN.UTF-8";

    // default timeout of backup job
    @ConfField public static int backup_job_default_timeout_ms = 86400 * 1000; // 1 day
    
    /*
     * storage_high_watermark_usage_percent limit the max capacity usage percent of a Backend storage path.
     * storage_min_left_capacity_bytes limit the minimum left capacity of a Backend storage path.
     * If both limitations are reached, this storage path can not be chose as tablet balance destination.
     * But for tablet recovery, we may exceed these limit for keeping data integrity as much as possible.
     */
    @ConfField public static double storage_high_watermark_usage_percent = 0.85;
    @ConfField public static double storage_min_left_capacity_bytes = 1000 * 1024 * 1024; // 1G

    // May be necessary to modify the following BRPC configurations in high concurrency scenarios. 
    // The number of concurrent requests BRPC can processed
    @ConfField public static int brpc_number_of_concurrent_requests_processed = 4096;

    // BRPC idle wait time (ms)
    @ConfField public static int brpc_idle_wait_max_time = 10000;
    
    /*
     * if set to false, auth check will be disable, in case some goes wrong with the new privilege system. 
     */
    @ConfField public static boolean enable_auth_check = true;
}
