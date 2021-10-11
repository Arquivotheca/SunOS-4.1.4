/*
 * @(#)dcptest_msg.c - Rev 1.1 - 10/31/94
 *
 * dcptest_msg.c:  message file for dcptest.c.
 *
 */

char *test_failed_msg = "Failed test for DCP.";
char *dcp_install_msg = "%s DCP chip is installed.";
char *no_device_msg   = "No %s device file.";
char *cant_make_msg   = "Cannot make /dev/des device file.";
char *no_rw_msg       = "%s not read/write character device file.";
char *file_nodcp_msg  = "%s exists, but no DCP!";
char *eeprom_err_msg  = "No configuration for DCP in EEPROM.";
char *system_err_msg  = "System does not support DCP.";
char *install_err_msg = "System does not support DCP.";
char *eeopen_msg      = "eeopen = %d\n";
char *eeopen_err_msg  = "Cannot open /dev/eeprom.";
char *lseek_err_msg   = "lseek failed.";
char *lseek_pass_msg  = "passed lseek: seek_ok = %x\n";
char *read_err_msg    = "read failed.";
char *val_msg         = "val = %d\n";
char *buf_msg         = "bug = %x\n";
char *no_config_msg   = "No configuration for DCP in EEPROM.";
char *bad_config_msg  = "System configuration incorrect in EEPROM.";
char *start_mode_msg  = "Starting %s mode test\n";
char *encrypt_err_msg = "Data was not encrypted.";
