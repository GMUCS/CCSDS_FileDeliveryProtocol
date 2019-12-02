

#include "protocol_handler.h"
#include "stdlib.h"
#include "mib.h"
#include "test.h"
#include "file_delivery_app.h"
#include "unit_tests.h"
#include "port.h"
#include "requests.h"



static int test_process_pdu_eof() {
//char *packet, Request *req, Response res
    
    DECLARE_NEW_TEST("testing process eof pdu");

    char *packet = malloc(2000);
    mock_eof_packet(packet, 1, 2);
    Request *req = mock_request();

    process_pdu_eof(packet, req, req->res);

    free(packet);
    ssp_cleanup_req(req);

    
    return 0;
}



static int test_wrong_id(FTP *app) {
    int error = 0;
    Response *res = mock_response();
    char packet[2000];

    int packet_index = mock_packet(packet, 2, 1);

    Request **req_container = &app->current_request; 
    process_pdu_header(packet, true, *res, req_container, app->request_list, app);
    Request *req = (*req_container);
    error = ASSERT_NULL("Test wrong id, Request should be NULL", req);
    error = ASSERT_EQUALS_INT("Length of request list should be 0", app->request_list->count, 0);

    free(res);
    return error;
}

static int test_correct_id(FTP *app) {
    int error = 0;
    Response *res = mock_response();
    char packet[2000];
    int packet_index = mock_packet(packet, 1, 2);

    Request **req_container = &app->current_request; 
    process_pdu_header(packet, true, *res, req_container, app->request_list, app);
    Request *req = (*req_container);

    error = ASSERT_NOT_NULL("Test wrong id, Request should not be NULL", req);
    error = ASSERT_EQUALS_INT("Length of request should be 1", app->request_list->count, 1);
    error = ASSERT_EQUALS_INT("request transaction number should equal", req->transaction_sequence_number, 1);
    error = ASSERT_EQUALS_INT("souce id should equal 2", req->dest_cfdp_id, 2);

    free(res);
    return error;
}

static int test_process_pdu_header() {

    int error = 0;
    FTP *app = init_ftp(1);

    error = test_wrong_id(app);
    error = test_correct_id(app);

    app->close = true;
    ssp_thread_join(app->server_handle);
    return error;
}


int protocol_handler_test() {
    int error = 0;
    //error = test_process_pdu_header();
    error = test_process_pdu_eof();
    return error;
}