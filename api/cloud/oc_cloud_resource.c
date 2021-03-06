/****************************************************************************
 *
 * Copyright 2019 Jozef Kralik All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
#ifdef OC_CLOUD

#include "oc_api.h"
#include "oc_cloud_internal.h"
#include "oc_config.h"
#include "oc_core_res.h"

#define OC_RSRVD_RES_TYPE_COAPCLOUDCONF "oic.r.coapcloudconf"
#define OC_RSRVD_URI_COAPCLOUDCONF "/CoapCloudConfResURI"
#define OC_RSRVD_ACCESSTOKEN "at"
#define OC_RSRVD_AUTHPROVIDER "apn"
#define OC_RSRVD_CISERVER "cis"
#define OC_RSRVD_SERVERID "sid"
#define OC_RSRVD_LAST_ERROR_CODE "clec"

static void
cloud_response(oc_cloud_context_t *ctx)
{
  oc_rep_start_root_object();
  oc_process_baseline_interface(ctx->cloud_conf);
  oc_rep_set_text_string(root, apn, (oc_string(ctx->store.auth_provider) != NULL
                                       ? oc_string(ctx->store.auth_provider)
                                       : ""));
  oc_rep_set_text_string(
    root, cis,
    (oc_string(ctx->store.ci_server) ? oc_string(ctx->store.ci_server) : ""));
  oc_rep_set_text_string(
    root, sid, (oc_string(ctx->store.sid) ? oc_string(ctx->store.sid) : ""));
  oc_rep_set_text_string(root, at, "");
  oc_rep_set_int(root, clec, (int)ctx->last_error);

  oc_rep_end_root_object();
}

static void
get_cloud(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)user_data;
  (void)interface;
  oc_cloud_context_t *ctx = oc_cloud_get_context(request->resource->device);
  if (!ctx) {
    oc_send_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  OC_DBG("GET request received");

  cloud_response(ctx);
  oc_send_response(request, OC_STATUS_OK);
}

static bool
cloud_update_from_request(oc_cloud_context_t *ctx, oc_request_t *request)
{
  bool changed = false;
  cloud_conf_update_t data;
  memset(&data, 0, sizeof(data));

  if (oc_rep_get_string(request->request_payload, OC_RSRVD_ACCESSTOKEN,
                        &data.access_token, &data.access_token_len)) {
    changed = true;
  }

  if (oc_rep_get_string(request->request_payload, OC_RSRVD_AUTHPROVIDER,
                        &data.auth_provider, &data.auth_provider_len)) {
    changed = true;
  }

  if (oc_rep_get_string(request->request_payload, OC_RSRVD_CISERVER,
                        &data.ci_server, &data.ci_server_len)) {
    changed = true;
  }

  // OCF 2.0 spec version added sid property.
  if (oc_rep_get_string(request->request_payload, OC_RSRVD_SERVERID, &data.sid,
                        &data.sid_len)) {
    changed = true;
  }

  if (changed) {
    cloud_update_by_resource(ctx, &data);
  }

  return changed;
}

static void
post_cloud(oc_request_t *request, oc_interface_mask_t interface,
           void *user_data)
{
  (void)user_data;
  oc_cloud_context_t *ctx = oc_cloud_get_context(request->resource->device);
  if (!ctx) {
    oc_send_response(request, OC_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }
  OC_DBG("POST request received");
  (void)interface;

  bool changed = cloud_update_from_request(ctx, request);
  cloud_response(ctx);
  oc_send_response(request,
                   changed ? OC_STATUS_CHANGED : OC_STATUS_NOT_MODIFIED);
}

void
oc_create_cloudconf_resource(size_t device)
{
  OC_DBG("oc_cloud_resource: Initializing CoAPCloudConf resource");

  oc_core_populate_resource(
    OCF_COAPCLOUDCONF, device, OC_RSRVD_URI_COAPCLOUDCONF,
    OC_IF_RW | OC_IF_BASELINE, OC_IF_RW,
    OC_SECURE | OC_DISCOVERABLE | OC_OBSERVABLE, get_cloud, 0, post_cloud, 0, 1,
    OC_RSRVD_RES_TYPE_COAPCLOUDCONF);
}
#else  /* OC_CLOUD*/
typedef int dummy_declaration;
#endif /* !OC_CLOUD */
