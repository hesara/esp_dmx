/**
 * @file rdm/utils/pd.h
 * @author Mitch Weisbrod
 * @brief // TODO
 *
 */
#pragma once

#include <stdbool.h>


#include "dmx/types.h"
#include "rdm/responder.h"
#include "rdm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rdm_pd_definition_s rdm_pd_definition_t;

typedef size_t (*rdm_response_handler_t)(dmx_port_t dmx_num,
                                         const rdm_pd_definition_t *definition,
                                         const rdm_header_t *header);

typedef struct rdm_pd_definition_s {
  rdm_pid_t pid;
  size_t alloc_size;
  uint8_t pid_cc;
  uint8_t ds;
  struct {
    struct {
      const char *format;
    } request, response;
  } get, set;
  rdm_response_handler_t response_handler;
  uint8_t pdl_size;
  uint32_t max_value;
  uint32_t min_value;
  uint32_t default_value;
  uint8_t units;
  uint8_t prefix;
  const char *description;
} rdm_pd_definition_t;

int rdm_pd_set_definition(dmx_port_t dmx_num, rdm_pid_t pid,
                          const rdm_pd_definition_t *definition);

bool rdm_pd_set_callback(dmx_port_t dmx_num, rdm_pid_t pid,
                         rdm_callback_t callback, void *context);

const rdm_pd_definition_t *rdm_pd_get_definition(dmx_port_t dmx_num,
                                                 rdm_pid_t pid);

const void *rdm_pd_add_variable(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                                rdm_pid_t pid, bool non_volatile,
                                const void *init_value, size_t size);

const void *rdm_pd_add_alias(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                             rdm_pid_t pid, bool non_volatile, rdm_pid_t alias,
                             size_t offset);

const void *rdm_pd_add_static(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                              rdm_pid_t pid, void *data);

const void *rdm_pd_get_pointer(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                               rdm_pid_t pid);

size_t rdm_pd_get(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                  rdm_pid_t pid, void *destination, size_t size);

size_t rdm_pd_set(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                  rdm_pid_t pid, const void *source, size_t size);

size_t rdm_pd_set_and_queue(dmx_port_t dmx_num, rdm_sub_device_t sub_device,
                            rdm_pid_t pid, const void *source, size_t size);

uint8_t rdm_pd_queue_size(dmx_port_t dmx_num);

rdm_pid_t rdm_pd_queue_pop(dmx_port_t dmx_num);

rdm_pid_t rdm_pd_queue_get_last_message(dmx_port_t dmx_num);

#ifdef __cplusplus
}
#endif
