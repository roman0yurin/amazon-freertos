/*
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file aws_iot_shadow_api.c
 * @brief Implements the user-facing functions of the Shadow library.
 */

/* Build using a config header, if provided. */
#ifdef AWS_IOT_CONFIG_FILE
    #include AWS_IOT_CONFIG_FILE
#endif

/* Standard includes. */
#include <string.h>

/* Shadow internal include. */
#include "private/aws_iot_shadow_internal.h"

/* JSON utilities include. */
#include "aws_iot_json_utils.h"

/* Require logging to be enabled for the MQTT library if logging is enabled for
 * the Shadow library. */
#if ( ( AWS_IOT_LOG_LEVEL_MQTT == AWS_IOT_LOG_NONE ) ||                                         \
    ( !defined( AWS_IOT_LOG_LEVEL_MQTT ) && !defined( AWS_IOT_LOG_LEVEL_GLOBAL ) ) ||           \
    ( !defined( AWS_IOT_LOG_LEVEL_MQTT ) && AWS_IOT_LOG_LEVEL_GLOBAL == AWS_IOT_LOG_NONE ) ) && \
    ( _LIBRARY_LOG_LEVEL > AWS_IOT_LOG_NONE )
    #error "Shadow library logging requires MQTT library logging to be enabled."
#endif

/* Validate Shadow configuration settings. */
#if AWS_IOT_SHADOW_ENABLE_ASSERTS != 0 && AWS_IOT_SHADOW_ENABLE_ASSERTS != 1
    #error "AWS_IOT_SHADOW_ENABLE_ASSERTS must be 0 or 1."
#endif
#if AWS_IOT_SHADOW_DEFAULT_MQTT_TIMEOUT_MS <= 0
    #error "AWS_IOT_SHADOW_DEFAULT_MQTT_TIMEOUT_MS cannot be 0 or negative."
#endif

/*-----------------------------------------------------------*/

static AwsIotShadowError_t _validateThingNameFlags( _shadowOperationType_t type,
                                                    const char * const pThingName,
                                                    size_t thingNameLength,
                                                    uint32_t flags,
                                                    const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                                    const AwsIotShadowReference_t * const pReference );

static AwsIotShadowError_t _validateDocumentInfo( _shadowOperationType_t type,
                                                  uint32_t flags,
                                                  const AwsIotShadowDocumentInfo_t * pDocumentInfo );

static AwsIotShadowError_t _setCallbackCommon( AwsIotMqttConnection_t mqttConnection,
                                               _shadowCallbackType_t type,
                                               const char * const pThingName,
                                               size_t thingNameLength,
                                               const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                               uint64_t timeoutMs );

static AwsIotShadowError_t _modifyCallbackSubscriptions( AwsIotMqttConnection_t mqttConnection,
                                                         _shadowCallbackType_t type,
                                                         _shadowSubscription_t * const pSubscription,
                                                         uint64_t timeoutMs,
                                                         _mqttOperationFunction_t mqttOperation );

static void _callbackWrapperCommon( _shadowCallbackType_t type,
                                    const _shadowSubscription_t * const pSubscription,
                                    AwsIotMqttCallbackParam_t * const pMessage );

static void _deltaCallbackWrapper( void * pArgument,
                                   AwsIotMqttCallbackParam_t * const pMessage );

static void _updatedCallbackWrapper( void * pArgument,
                                     AwsIotMqttCallbackParam_t * const pMessage );

/*-----------------------------------------------------------*/

uint64_t _AwsIotShadowMqttTimeout = AWS_IOT_SHADOW_DEFAULT_MQTT_TIMEOUT_MS;

#if _LIBRARY_LOG_LEVEL > _AWS_IOT_LOG_NONE
    const char * const _pAwsIotShadowCallbackNames[] =
    {
        "DELTA",
        "UPDATED"
    };
#endif

/*-----------------------------------------------------------*/

static AwsIotShadowError_t _validateThingNameFlags( _shadowOperationType_t type,
                                                    const char * const pThingName,
                                                    size_t thingNameLength,
                                                    uint32_t flags,
                                                    const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                                    const AwsIotShadowReference_t * const pReference )
{
    /* Type is not used when logging is disabled. */
    ( void ) type;

    /* Check Thing Name. */
    if( ( pThingName == NULL ) || ( thingNameLength == 0 ) )
    {
        AwsIotLogError( "Thing name for Shadow %s cannot be NULL or have length 0.",
                        _pAwsIotShadowOperationNames[ type ] );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    if( thingNameLength > _MAX_THING_NAME_LENGTH )
    {
        AwsIotLogError( "Thing Name length of %lu exceeds the maximum allowed"
                        "length of %d.",
                        ( unsigned long ) thingNameLength,
                        _MAX_THING_NAME_LENGTH );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check the waitable operation flag. */
    if( ( flags & AWS_IOT_SHADOW_FLAG_WAITABLE ) == AWS_IOT_SHADOW_FLAG_WAITABLE )
    {
        /* Check that a reference pointer is provided for a waitable operation. */
        if( pReference == NULL )
        {
            AwsIotLogError( "Reference must be set for a waitable Shadow %s.",
                            _pAwsIotShadowOperationNames[ type ] );

            return AWS_IOT_SHADOW_BAD_PARAMETER;
        }

        /* A callback should not be set for a waitable operation. */
        if( pCallbackInfo != NULL )
        {
            AwsIotLogError( "Callback should not be set for a waitable Shadow %s.",
                            _pAwsIotShadowOperationNames[ type ] );

            return AWS_IOT_SHADOW_BAD_PARAMETER;
        }
    }

    /* A callback info must be passed to a non-waitable GET. */
    if( ( type == _SHADOW_GET ) &&
        ( ( flags & AWS_IOT_SHADOW_FLAG_WAITABLE ) == 0 ) &&
        ( pCallbackInfo == NULL ) )
    {
        AwsIotLogError( "Callback info must be provided for non-waitable Shadow GET." );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check that a callback function is set. */
    if( ( pCallbackInfo != NULL ) &&
        ( pCallbackInfo->function == NULL ) )
    {
        AwsIotLogError( "Callback function must be set for Shadow %s callback.",
                        _pAwsIotShadowOperationNames[ type ] );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    return AWS_IOT_SHADOW_SUCCESS;
}

/*-----------------------------------------------------------*/

static AwsIotShadowError_t _validateDocumentInfo( _shadowOperationType_t type,
                                                  uint32_t flags,
                                                  const AwsIotShadowDocumentInfo_t * pDocumentInfo )
{
    /* This function should only be called for Shadow GET or UPDATE. */
    AwsIotShadow_Assert( ( type == _SHADOW_GET ) || ( type == _SHADOW_UPDATE ) );

    /* Check QoS. */
    if( ( pDocumentInfo->QoS < 0 ) || ( pDocumentInfo->QoS > 1 ) )
    {
        AwsIotLogError( "QoS for Shadow %d must be 0 or 1.",
                        _pAwsIotShadowOperationNames[ type ] );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check the retry parameters. */
    if( pDocumentInfo->retryLimit < 0 )
    {
        AwsIotLogError( "Retry limit of Shadow %s cannot be less than 0.",
                        _pAwsIotShadowOperationNames[ type ] );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }
    else if( pDocumentInfo->retryLimit > 0 )
    {
        if( pDocumentInfo->retryMs == 0 )
        {
            AwsIotLogError( "Retry time of Shadow %s must be positive.",
                            _pAwsIotShadowOperationNames[ type ] );

            return AWS_IOT_SHADOW_BAD_PARAMETER;
        }
    }

    /* Check members relevant to a Shadow GET. */
    if( type == _SHADOW_GET )
    {
        /* Check memory allocation function for waitable GET. */
        if( ( ( flags & AWS_IOT_SHADOW_FLAG_WAITABLE ) == AWS_IOT_SHADOW_FLAG_WAITABLE ) &&
            ( pDocumentInfo->get.mallocDocument == NULL ) )
        {
            AwsIotLogError( "Memory allocation function must be set for waitable Shadow GET." );

            return AWS_IOT_SHADOW_BAD_PARAMETER;
        }
    }
    /* Check members relevant to a Shadow UPDATE. */
    else
    {
        /* Check UPDATE document pointer and length. */
        if( ( pDocumentInfo->update.pUpdateDocument == NULL ) ||
            ( pDocumentInfo->update.updateDocumentLength == 0 ) )
        {
            AwsIotLogError( "Shadow document for Shadow UPDATE cannot be NULL or"
                            " have length 0." );

            return AWS_IOT_SHADOW_BAD_PARAMETER;
        }
    }

    return AWS_IOT_SHADOW_SUCCESS;
}

/*-----------------------------------------------------------*/

static AwsIotShadowError_t _setCallbackCommon( AwsIotMqttConnection_t mqttConnection,
                                               _shadowCallbackType_t type,
                                               const char * const pThingName,
                                               size_t thingNameLength,
                                               const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                               uint64_t timeoutMs )
{
    AwsIotShadowError_t status = AWS_IOT_SHADOW_SUCCESS;
    _shadowSubscription_t * pSubscription = NULL;

    /* Check parameters. */
    if( _validateThingNameFlags( type + _SHADOW_OPERATION_COUNT, /* Shadow callbacks are enumerated after the operations. */
                                 pThingName,
                                 thingNameLength,
                                 0,
                                 pCallbackInfo,
                                 NULL ) != AWS_IOT_SHADOW_SUCCESS )
    {
        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    AwsIotLogDebug( "Processing Shadow %s callback for %.*s.",
                    _pAwsIotShadowCallbackNames[ type ],
                    thingNameLength,
                    pThingName );

    /* Lock the subscription list mutex to check for an existing subscription
     * object. */
    AwsIotMutex_Lock( &( _AwsIotShadowSubscriptions.mutex ) );

    /* Check for an existing subscription. This function will attempt to allocate
     * a new subscription if not found. */
    pSubscription = AwsIotShadowInternal_FindSubscription( pThingName,
                                                           thingNameLength );

    if( pSubscription == NULL )
    {
        /* No existing subscription was found, and no new subscription could be
         * allocated. */
        status = AWS_IOT_SHADOW_NO_MEMORY;
    }
    else
    {
        /* Check for an existing callback. */
        if( pSubscription->callbacks[ type ].function != NULL )
        {
            /* Replace existing callback. */
            if( pCallbackInfo != NULL )
            {
                AwsIotLogDebug( "Found existing %s callback for %.*s. Replacing callback.",
                                _pAwsIotShadowCallbackNames[ type ],
                                thingNameLength,
                                pThingName );

                pSubscription->callbacks[ type ] = *pCallbackInfo;
            }
            /* Remove existing callback. */
            else
            {
                AwsIotLogDebug( "Removing existing %s callback for %.*s.",
                                _pAwsIotShadowCallbackNames[ type ],
                                thingNameLength,
                                pThingName );

                /* Clear the callback information and unsubscribe. */
                ( void ) memset( &( pSubscription->callbacks[ type ] ),
                                 0x00,
                                 sizeof( AwsIotShadowCallbackInfo_t ) );
                ( void ) _modifyCallbackSubscriptions( mqttConnection,
                                                       type,
                                                       pSubscription,
                                                       timeoutMs,
                                                       AwsIotMqtt_TimedUnsubscribe );

                /* Check if this subscription object can be removed. */
                AwsIotShadowInternal_RemoveSubscription( pSubscription, NULL );
            }
        }
        /* No existing callback. */
        else
        {
            /* Add new callback. */
            if( pCallbackInfo != NULL )
            {
                AwsIotLogDebug( "Adding new %s callback for %.*s.",
                                _pAwsIotShadowCallbackNames[ type ],
                                thingNameLength,
                                pThingName );

                pSubscription->callbacks[ type ] = *pCallbackInfo;
                status = _modifyCallbackSubscriptions( mqttConnection,
                                                       type,
                                                       pSubscription,
                                                       timeoutMs,
                                                       AwsIotMqtt_TimedSubscribe );
            }
            /* Do nothing; set return value to success. */
            else
            {
                status = AWS_IOT_SHADOW_SUCCESS;
            }
        }
    }

    AwsIotMutex_Unlock( &( _AwsIotShadowSubscriptions.mutex ) );

    return status;
}

/*-----------------------------------------------------------*/

static AwsIotShadowError_t _modifyCallbackSubscriptions( AwsIotMqttConnection_t mqttConnection,
                                                         _shadowCallbackType_t type,
                                                         _shadowSubscription_t * const pSubscription,
                                                         uint64_t timeoutMs,
                                                         _mqttOperationFunction_t mqttOperation )
{
    AwsIotShadowError_t status = AWS_IOT_SHADOW_SUCCESS;
    AwsIotMqttError_t mqttStatus = AWS_IOT_MQTT_STATUS_PENDING;
    AwsIotMqttSubscription_t subscription = AWS_IOT_MQTT_SUBSCRIPTION_INITIALIZER;
    char * pTopicFilter = NULL;
    uint16_t operationTopicLength = 0;

    /* Lookup table for Shadow callback suffixes. */
    const char * const pCallbackSuffix[ _SHADOW_CALLBACK_COUNT ] =
    {
        _SHADOW_DELTA_SUFFIX,   /* Delta callback. */
        _SHADOW_UPDATED_SUFFIX  /* Updated callback. */
    };

    /* Lookup table for Shadow callback suffix lengths. */
    const uint16_t pCallbackSuffixLength[ _SHADOW_CALLBACK_COUNT ] =
    {
        _SHADOW_DELTA_SUFFIX_LENGTH,   /* Delta callback. */
        _SHADOW_UPDATED_SUFFIX_LENGTH  /* Updated callback. */
    };

    /* Lookup table for Shadow callback function wrappers. */
    const _mqttCallbackFunction_t pCallbackWrapper[ _SHADOW_CALLBACK_COUNT ] =
    {
        _deltaCallbackWrapper,   /* Delta callback. */
        _updatedCallbackWrapper, /* Updated callback. */
    };

    /* MQTT operation may only be subscribe or unsubscribe. */
    AwsIotShadow_Assert( ( mqttOperation == AwsIotMqtt_TimedSubscribe ) ||
                         ( mqttOperation == AwsIotMqtt_TimedUnsubscribe ) );

    /* Use the subscription's topic buffer if available. */
    if( pSubscription->pTopicBuffer != NULL )
    {
        pTopicFilter = pSubscription->pTopicBuffer;
    }

    /* Generate the prefix portion of the Shadow callback topic filter. Both
     * callbacks share the same callback as the Shadow Update operation. */
    if( AwsIotShadowInternal_GenerateShadowTopic( _SHADOW_UPDATE,
                                                  pSubscription->pThingName,
                                                  pSubscription->thingNameLength,
                                                  &pTopicFilter,
                                                  &operationTopicLength ) != AWS_IOT_SHADOW_SUCCESS )
    {
        return AWS_IOT_SHADOW_NO_MEMORY;
    }

    /* Place the callback suffix in the topic filter. */
    ( void ) memcpy( pTopicFilter + operationTopicLength,
                     pCallbackSuffix[ type ],
                     pCallbackSuffixLength[ type ] );

    AwsIotLogDebug( "%s subscription for %.*s",
                    mqttOperation == AwsIotMqtt_TimedSubscribe ? "Adding" : "Removing",
                    operationTopicLength + pCallbackSuffixLength[ type ],
                    pTopicFilter );

    /* Set the members of the MQTT subscription. */
    subscription.QoS = 1;
    subscription.pTopicFilter = pTopicFilter;
    subscription.topicFilterLength = ( uint16_t ) ( operationTopicLength + pCallbackSuffixLength[ type ] );
    subscription.callback.param1 = ( void * ) pSubscription;
    subscription.callback.function = pCallbackWrapper[ type ];

    /* Call the MQTT operation function. */
    mqttStatus = mqttOperation( mqttConnection,
                                &subscription,
                                1,
                                0,
                                timeoutMs );

    /* Check the result of the MQTT operation. */
    if( mqttStatus != AWS_IOT_MQTT_SUCCESS )
    {
        AwsIotLogError( "Failed to %s callback for %.*s %s callback, error %s.",
                        mqttOperation == AwsIotMqtt_TimedSubscribe ? "subscribe to" : "unsubscribe from",
                        pSubscription->thingNameLength,
                        pSubscription->pThingName,
                        _pAwsIotShadowCallbackNames[ type ],
                        AwsIotMqtt_strerror( mqttStatus ) );

        /* Convert the MQTT "NO MEMORY" error to a Shadow "NO MEMORY" error. */
        if( mqttStatus == AWS_IOT_MQTT_NO_MEMORY )
        {
            status = AWS_IOT_SHADOW_NO_MEMORY;
        }
        else
        {
            status = AWS_IOT_SHADOW_MQTT_ERROR;
        }
    }
    else
    {
        AwsIotLogDebug( "Successfully %s %.*s Shadow %s callback.",
                        mqttOperation == AwsIotMqtt_TimedSubscribe ? "subscribed to" : "unsubscribed from",
                        pSubscription->thingNameLength,
                        pSubscription->pThingName,
                        _pAwsIotShadowCallbackNames[ type ] );
    }

    /* MQTT subscribe should check the subscription topic buffer. */
    if( mqttOperation == AwsIotMqtt_TimedSubscribe )
    {
        /* If the current subscription has no topic buffer, assign it the current
         * topic buffer. Otherwise, free the current topic buffer. */
        if( pSubscription->pTopicBuffer == NULL )
        {
            pSubscription->pTopicBuffer = pTopicFilter;
        }
        else
        {
            AwsIotShadow_FreeString( pTopicFilter );
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

static void _callbackWrapperCommon( _shadowCallbackType_t type,
                                    const _shadowSubscription_t * const pSubscription,
                                    AwsIotMqttCallbackParam_t * const pMessage )
{
    AwsIotShadowCallbackParam_t callbackParam = { 0 };

    /* Ensure that a callback function is set. */
    AwsIotShadow_Assert( pSubscription->callbacks[ type ].function != NULL );

    /* Set the members of the callback param. */
    callbackParam.callbackType = type + _SHADOW_OPERATION_COUNT; /* Shadow callbacks are enumerated after the operations. */
    callbackParam.pThingName = pSubscription->pThingName;
    callbackParam.thingNameLength = pSubscription->thingNameLength;
    callbackParam.callback.pDocument = pMessage->message.info.pPayload;
    callbackParam.callback.documentLength = pMessage->message.info.payloadLength;

    /* Invoke the callback function. */
    pSubscription->callbacks[ type ].function( pSubscription->callbacks[ type ].param1,
                                                          &callbackParam );
}

/*-----------------------------------------------------------*/

static void _deltaCallbackWrapper( void * pArgument,
                                   AwsIotMqttCallbackParam_t * const pMessage )
{
    _callbackWrapperCommon( _DELTA_CALLBACK, pArgument, pMessage );
}

/*-----------------------------------------------------------*/

static void _updatedCallbackWrapper( void * pArgument,
                                     AwsIotMqttCallbackParam_t * const pMessage )
{
    _callbackWrapperCommon( _UPDATED_CALLBACK, pArgument, pMessage );
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_Init( uint64_t mqttTimeout )
{
    /* Create the Shadow pending operation list. */
    if( AwsIotShadowInternal_CreatePendingOperationList() != AWS_IOT_SHADOW_SUCCESS )
    {
        AwsIotLogError( "Failed to create Shadow pending operation list." );

        return AWS_IOT_SHADOW_INIT_FAILED;
    }

    /* Create the Shadow subscription list. */
    if( AwsIotShadowInternal_CreateSubscriptionList() != AWS_IOT_SHADOW_SUCCESS )
    {
        AwsIotLogError( "Failed to create Shadow subscription list." );
        AwsIotList_Destroy( &_AwsIotShadowPendingOperations );

        return AWS_IOT_SHADOW_INIT_FAILED;
    }

    /* Save the MQTT timeout option. */
    if( mqttTimeout != 0 )
    {
        _AwsIotShadowMqttTimeout = mqttTimeout;
    }

    AwsIotLogInfo( "Shadow library successfully initialized." );

    return AWS_IOT_SHADOW_SUCCESS;
}

/*-----------------------------------------------------------*/

void AwsIotShadow_Cleanup( void )
{
    /* Remove and free all items in the Shadow pending operation list, then destroy
     * the list. */
    AwsIotList_RemoveAllMatches( &_AwsIotShadowPendingOperations,
                                 NULL,
                                 NULL,
                                 AwsIotShadowInternal_DestroyOperation );
    AwsIotList_Destroy( &_AwsIotShadowPendingOperations );

    /* Remove and free all items in the Shadow subscription list, then destroy
     * the list. */
    AwsIotList_RemoveAllMatches( &_AwsIotShadowSubscriptions,
                                 NULL,
                                 NULL,
                                 AwsIotShadowInternal_DestroySubscription );
    AwsIotList_Destroy( &_AwsIotShadowSubscriptions );

    /* Restore the default MQTT timeout. */
    _AwsIotShadowMqttTimeout = AWS_IOT_SHADOW_DEFAULT_MQTT_TIMEOUT_MS;

    AwsIotLogInfo( "Shadow library cleanup done." );
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_Delete( AwsIotMqttConnection_t mqttConnection,
                                         const char * const pThingName,
                                         size_t thingNameLength,
                                         uint32_t flags,
                                         const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                         AwsIotShadowReference_t * const pDeleteRef )
{
    _shadowOperation_t * pOperation = NULL;
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;

    /* Validate the Thing Name and flags for Shadow DELETE. */
    if( _validateThingNameFlags( _SHADOW_DELETE,
                                 pThingName,
                                 thingNameLength,
                                 flags,
                                 pCallbackInfo,
                                 pDeleteRef ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* The Thing Name or some flag was invalid. */
        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Allocate a new Shadow operation for DELETE. */
    if( AwsIotShadowInternal_CreateOperation( &pOperation,
                                              _SHADOW_DELETE,
                                              flags,
                                              pCallbackInfo ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* No memory for a new Shadow operation. */
        return AWS_IOT_SHADOW_NO_MEMORY;
    }

    /* Check the members set by Shadow operation creation. */
    AwsIotShadow_Assert( pOperation != NULL );
    AwsIotShadow_Assert( pOperation->type == _SHADOW_DELETE );
    AwsIotShadow_Assert( pOperation->flags == flags );
    AwsIotShadow_Assert( pOperation->status == AWS_IOT_SHADOW_STATUS_PENDING );

    /* Set the reference if provided. This must be done before the Shadow operation
     * is processed. */
    if( pDeleteRef != NULL )
    {
        *pDeleteRef = pOperation;
    }

    /* Process the Shadow operation. This subscribes to any required topics and
     * sends the MQTT message for the Shadow operation. */
    status = AwsIotShadowInternal_ProcessOperation( mqttConnection,
                                                    pThingName,
                                                    thingNameLength,
                                                    pOperation,
                                                    NULL );

    /* If the Shadow operation failed, clear the now invalid reference. */
    if( ( status != AWS_IOT_SHADOW_STATUS_PENDING ) && ( pDeleteRef != NULL ) )
    {
        *pDeleteRef = AWS_IOT_SHADOW_REFERENCE_INITIALIZER;
    }

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_TimedDelete( AwsIotMqttConnection_t mqttConnection,
                                              const char * const pThingName,
                                              size_t thingNameLength,
                                              uint32_t flags,
                                              uint64_t timeoutMs )
{
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowReference_t deleteRef = AWS_IOT_SHADOW_REFERENCE_INITIALIZER;

    /* Set the waitable flag. */
    flags |= AWS_IOT_SHADOW_FLAG_WAITABLE;

    /* Call the asynchronous Shadow delete function. */
    status = AwsIotShadow_Delete( mqttConnection,
                                  pThingName,
                                  thingNameLength,
                                  flags,
                                  NULL,
                                  &deleteRef );

    /* Wait for the Shadow delete operation to complete. */
    if( status == AWS_IOT_SHADOW_STATUS_PENDING )
    {
        status = AwsIotShadow_Wait( deleteRef, timeoutMs, NULL, NULL );
    }

    /* Ensure that a status was set. */
    AwsIotShadow_Assert( status != AWS_IOT_SHADOW_STATUS_PENDING );

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_Get( AwsIotMqttConnection_t mqttConnection,
                                      const AwsIotShadowDocumentInfo_t * const pGetInfo,
                                      uint32_t flags,
                                      const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                      AwsIotShadowReference_t * const pGetRef )
{
    _shadowOperation_t * pOperation = NULL;
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;

    /* Validate the Thing Name and flags for Shadow GET. */
    if( _validateThingNameFlags( _SHADOW_GET,
                                 pGetInfo->pThingName,
                                 pGetInfo->thingNameLength,
                                 flags,
                                 pCallbackInfo,
                                 pGetRef ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* The Thing Name or some flag was invalid. */
        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Validate the document info for Shadow GET. */
    if( _validateDocumentInfo( _SHADOW_GET,
                               flags,
                               pGetInfo ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* Document info was invalid. */
        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Allocate a new Shadow operation for GET. */
    if( AwsIotShadowInternal_CreateOperation( &pOperation,
                                              _SHADOW_GET,
                                              flags,
                                              pCallbackInfo ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* No memory for a new Shadow operation. */
        return AWS_IOT_SHADOW_NO_MEMORY;
    }

    /* Check the members set by Shadow operation creation. */
    AwsIotShadow_Assert( pOperation != NULL );
    AwsIotShadow_Assert( pOperation->type == _SHADOW_GET );
    AwsIotShadow_Assert( pOperation->flags == flags );
    AwsIotShadow_Assert( pOperation->status == AWS_IOT_SHADOW_STATUS_PENDING );

    /* Copy the memory allocation function. */
    pOperation->get.mallocDocument = pGetInfo->get.mallocDocument;

    /* Set the reference if provided. This must be done before the Shadow operation
     * is processed. */
    if( pGetRef != NULL )
    {
        *pGetRef = pOperation;
    }

    /* Process the Shadow operation. This subscribes to any required topics and
     * sends the MQTT message for the Shadow operation. */
    status = AwsIotShadowInternal_ProcessOperation( mqttConnection,
                                                    pGetInfo->pThingName,
                                                    pGetInfo->thingNameLength,
                                                    pOperation,
                                                    pGetInfo );

    /* If the Shadow operation failed, clear the now invalid reference. */
    if( ( status != AWS_IOT_SHADOW_STATUS_PENDING ) && ( pGetRef != NULL ) )
    {
        *pGetRef = AWS_IOT_SHADOW_REFERENCE_INITIALIZER;
    }

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_TimedGet( AwsIotMqttConnection_t mqttConnection,
                                           const AwsIotShadowDocumentInfo_t * const pGetInfo,
                                           uint32_t flags,
                                           uint64_t timeoutMs,
                                           const char ** const pShadowDocument,
                                           size_t * const pShadowDocumentLength )
{
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowReference_t getRef = AWS_IOT_SHADOW_REFERENCE_INITIALIZER;

    /* Set the waitable flag. */
    flags |= AWS_IOT_SHADOW_FLAG_WAITABLE;

    /* Call the asynchronous Shadow get function. */
    status = AwsIotShadow_Get( mqttConnection,
                               pGetInfo,
                               flags,
                               NULL,
                               &getRef );

    /* Wait for the Shadow get operation to complete. */
    if( status == AWS_IOT_SHADOW_STATUS_PENDING )
    {
        status = AwsIotShadow_Wait( getRef,
                                    timeoutMs,
                                    pShadowDocument,
                                    pShadowDocumentLength );
    }

    /* Ensure that a status was set. */
    AwsIotShadow_Assert( status != AWS_IOT_SHADOW_STATUS_PENDING );

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_Update( AwsIotMqttConnection_t mqttConnection,
                                         const AwsIotShadowDocumentInfo_t * const pUpdateInfo,
                                         uint32_t flags,
                                         const AwsIotShadowCallbackInfo_t * const pCallbackInfo,
                                         AwsIotShadowReference_t * const pUpdateRef )
{
    _shadowOperation_t * pOperation = NULL;
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;
    const char * pClientToken = NULL;
    size_t clientTokenLength = 0;

    /* Validate the Thing Name and flags for Shadow UPDATE. */
    if( _validateThingNameFlags( _SHADOW_UPDATE,
                                 pUpdateInfo->pThingName,
                                 pUpdateInfo->thingNameLength,
                                 flags,
                                 pCallbackInfo,
                                 pUpdateRef ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* The Thing Name or some flag was invalid. */
        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Validate the document info for Shadow UPDATE. */
    if( _validateDocumentInfo( _SHADOW_UPDATE,
                               flags,
                               pUpdateInfo ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* Document info was invalid. */
        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check UPDATE document for a client token. */
    if( AwsIotJsonUtils_FindJsonValue( pUpdateInfo->update.pUpdateDocument,
                                       pUpdateInfo->update.updateDocumentLength,
                                       _CLIENT_TOKEN_KEY,
                                       _CLIENT_TOKEN_KEY_LENGTH,
                                       &pClientToken,
                                       &clientTokenLength ) == false )
    {
        AwsIotLogError( "Shadow document for Shadow UPDATE must have a %s key.",
                        _CLIENT_TOKEN_KEY );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check the client token length. It must be greater than the length of its
     * enclosing double quotes (2) and less than the maximum allowed by the Shadow
     * service. */
    if( ( clientTokenLength < 2 ) || ( clientTokenLength > _MAX_CLIENT_TOKEN_LENGTH ) )
    {
        AwsIotLogError( "Client token length must be between 2 and %d (including "
                        "enclosing quotes).", _MAX_CLIENT_TOKEN_LENGTH + 2 );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Allocate a new Shadow operation for UPDATE. */
    if( AwsIotShadowInternal_CreateOperation( &pOperation,
                                              _SHADOW_UPDATE,
                                              flags,
                                              pCallbackInfo ) != AWS_IOT_SHADOW_SUCCESS )
    {
        /* No memory for a new Shadow operation. */
        return AWS_IOT_SHADOW_NO_MEMORY;
    }

    /* Check the members set by Shadow operation creation. */
    AwsIotShadow_Assert( pOperation != NULL );
    AwsIotShadow_Assert( pOperation->type == _SHADOW_UPDATE );
    AwsIotShadow_Assert( pOperation->flags == flags );
    AwsIotShadow_Assert( pOperation->status == AWS_IOT_SHADOW_STATUS_PENDING );

    /* Allocate memory for the client token. */
    pOperation->update.pClientToken = AwsIotShadow_MallocString( clientTokenLength );

    if( pOperation->update.pClientToken == NULL )
    {
        AwsIotLogError( "Failed to allocate memory for Shadow update client token." );
        AwsIotShadowInternal_DestroyOperation( pOperation );

        return AWS_IOT_SHADOW_NO_MEMORY;
    }

    /* Copy the client token. The client token must be copied in case the application
     * frees the buffer containing it. */
    ( void ) memcpy( ( void * ) pOperation->update.pClientToken,
                     pClientToken,
                     clientTokenLength );
    pOperation->update.clientTokenLength = clientTokenLength;

    /* Set the reference if provided. This must be done before the Shadow operation
     * is processed. */
    if( pUpdateRef != NULL )
    {
        *pUpdateRef = pOperation;
    }

    /* Process the Shadow operation. This subscribes to any required topics and
     * sends the MQTT message for the Shadow operation. */
    status = AwsIotShadowInternal_ProcessOperation( mqttConnection,
                                                    pUpdateInfo->pThingName,
                                                    pUpdateInfo->thingNameLength,
                                                    pOperation,
                                                    pUpdateInfo );

    /* If the Shadow operation failed, clear the now invalid reference. */
    if( ( status != AWS_IOT_SHADOW_STATUS_PENDING ) && ( pUpdateRef != NULL ) )
    {
        *pUpdateRef = AWS_IOT_SHADOW_REFERENCE_INITIALIZER;
    }

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_TimedUpdate( AwsIotMqttConnection_t mqttConnection,
                                              const AwsIotShadowDocumentInfo_t * const pUpdateInfo,
                                              uint32_t flags,
                                              uint64_t timeoutMs )
{
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowReference_t updateRef = AWS_IOT_SHADOW_REFERENCE_INITIALIZER;

    /* Set the waitable flag. */
    flags |= AWS_IOT_SHADOW_FLAG_WAITABLE;

    /* Call the asynchronous Shadow update function. */
    status = AwsIotShadow_Update( mqttConnection,
                                  pUpdateInfo,
                                  flags,
                                  NULL,
                                  &updateRef );

    /* Wait for the Shadow update operation to complete. */
    if( status == AWS_IOT_SHADOW_STATUS_PENDING )
    {
        status = AwsIotShadow_Wait( updateRef, timeoutMs, NULL, NULL );
    }

    /* Ensure that a status was set. */
    AwsIotShadow_Assert( status != AWS_IOT_SHADOW_STATUS_PENDING );

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_Wait( AwsIotShadowReference_t reference,
                                       uint64_t timeoutMs,
                                       const char ** const pShadowDocument,
                                       size_t * const pShadowDocumentLength )
{
    AwsIotShadowError_t status = AWS_IOT_SHADOW_STATUS_PENDING;
    _shadowOperation_t * pOperation = ( _shadowOperation_t * ) reference;

    /* Check that reference is set. */
    if( pOperation == NULL )
    {
        AwsIotLogError( "Reference cannot be NULL." );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check that reference is waitable. */
    if( ( pOperation->flags & AWS_IOT_SHADOW_FLAG_WAITABLE ) == 0 )
    {
        AwsIotLogError( "Reference is not a waitable Shadow operation." );

        return AWS_IOT_SHADOW_BAD_PARAMETER;
    }

    /* Check that output parameters are set for a Shadow GET. */
    if( pOperation->type == _SHADOW_GET )
    {
        if( ( pShadowDocument == NULL ) || ( pShadowDocumentLength == NULL ) )
        {
            AwsIotLogError( "Output buffer and size pointer must be set for Shadow GET." );

            return AWS_IOT_SHADOW_BAD_PARAMETER;
        }
    }

    /* Wait for a response to the Shadow operation. */
    if( AwsIotSemaphore_TimedWait( &( pOperation->notify.waitSemaphore ),
                                   timeoutMs ) == true )
    {
        status = pOperation->status;
    }
    else
    {
        status = AWS_IOT_SHADOW_TIMEOUT;
    }

    /* Remove the completed operation from the pending operation list. */
    AwsIotMutex_Lock( &( _AwsIotShadowPendingOperations.mutex ) );
    AwsIotList_Remove( &_AwsIotShadowPendingOperations, pOperation );
    AwsIotMutex_Unlock( &( _AwsIotShadowPendingOperations.mutex ) );

    /* Decrement the reference count. This also removes subscriptions if the
     * count reaches 0. */
    AwsIotMutex_Lock( &_AwsIotShadowSubscriptions.mutex );
    AwsIotShadowInternal_DecrementReferences( pOperation,
                                              pOperation->pSubscription->pTopicBuffer,
                                              NULL );
    AwsIotMutex_Unlock( &_AwsIotShadowSubscriptions.mutex );

    /* Set the output parameters for Shadow GET. */
    if( pOperation->type == _SHADOW_GET )
    {
        *pShadowDocument = pOperation->get.pDocument;
        *pShadowDocumentLength = pOperation->get.documentLength;
    }

    /* Destroy the Shadow operation. */
    AwsIotShadowInternal_DestroyOperation( pOperation );

    return status;
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_SetDeltaCallback( AwsIotMqttConnection_t mqttConnection,
                                                   const char * const pThingName,
                                                   size_t thingNameLength,
                                                   uint32_t flags,
                                                   const AwsIotShadowCallbackInfo_t * const pDeltaCallback,
                                                   uint64_t timeoutMs )
{
    /* Flags are currently not used by this function. */
    ( void ) flags;

    return _setCallbackCommon( mqttConnection,
                               _DELTA_CALLBACK,
                               pThingName,
                               thingNameLength,
                               pDeltaCallback,
                               timeoutMs );
}

/*-----------------------------------------------------------*/

AwsIotShadowError_t AwsIotShadow_SetUpdatedCallback( AwsIotMqttConnection_t mqttConnection,
                                                     const char * const pThingName,
                                                     size_t thingNameLength,
                                                     uint32_t flags,
                                                     const AwsIotShadowCallbackInfo_t * const pUpdatedCallback,
                                                     uint64_t timeoutMs )
{
    /* Flags are currently not used by this function. */
    ( void ) flags;

    return _setCallbackCommon( mqttConnection,
                               _UPDATED_CALLBACK,
                               pThingName,
                               thingNameLength,
                               pUpdatedCallback,
                               timeoutMs );
}

/*-----------------------------------------------------------*/