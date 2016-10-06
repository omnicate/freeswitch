#include <ks.h>
#include <tap.h>

int main() {
  int err = 0;
  char v4[48] = {0}, v6[48] = {0};
  int mask = 0, have_v4 = 0, have_v6 = 0;
  int A_port = 5998, B_port = 5999;
  dht_handle_t *A_h = NULL, *B_h = NULL;
  ks_dht_af_flag_t af_flags = 0;
  
  err = ks_init();
  ok(!err);

  err = ks_find_local_ip(v4, sizeof(v4), &mask, AF_INET, NULL);
  ok(err == KS_STATUS_SUCCESS);
  have_v4 = !zstr_buf(v4);
  
  err = ks_find_local_ip(v6, sizeof(v6), NULL, AF_INET6, NULL);
  ok(err == KS_STATUS_SUCCESS);

  have_v6 = !zstr_buf(v6);

  ok(have_v4 || have_v6);
  if (have_v4) {
    af_flags |= KS_DHT_AF_INET4;
  }
  diag("Adding local bind ipv4 of (%s) %d\n", v4, have_v4);

  if (have_v6) {
    af_flags |= KS_DHT_AF_INET6;
  }
  diag("Adding local bind ipv6 of (%s) %d\n", v6, have_v6);

  err = ks_dht_init(&A_h, af_flags, NULL, A_port);
  ok(err == KS_STATUS_SUCCESS);

  if (have_v4) {
    err = ks_dht_add_ip(A_h, v4, A_port);
    ok(err == KS_STATUS_SUCCESS);
  }

  if (have_v6) {
    err = ks_dht_add_ip(A_h, v6, A_port);
    ok(err == KS_STATUS_SUCCESS);
  }

  err = ks_dht_init(&B_h, af_flags, NULL, B_port);
  ok(err == KS_STATUS_SUCCESS);

  if (have_v4) {
    err = ks_dht_add_ip(B_h, v4, B_port);
    ok(err == KS_STATUS_SUCCESS);
  }

  if (have_v6) {
    err = ks_dht_add_ip(B_h, v6, B_port);
    ok(err == KS_STATUS_SUCCESS);
  }

  /* Start test series */


  /* Cleanup and shutdown */

  todo("ks_dht_stop()");
  todo("ks_dht_destroy()");
  
  err = ks_shutdown();
  ok(!err);
  
  done_testing();
}
