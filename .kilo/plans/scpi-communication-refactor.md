# SCPI Communication Architecture Refactor - Technical Implementation Plan

## Overview

This plan implements a clean separation of concerns between communication and system tasks, following the specified architectural workflow with proper state machines for message extraction and command processing. The design is structured for seamless integration with the [SCPI parser library](https://www.jaybee.cz/scpi-parser/) (https://github.com/j123b567/scpi-parser).

## Current Architecture Analysis

**Existing Components:**
- `task_comm.c` - Currently combines message extraction AND SCPI parsing (violates separation)
- `usart_hw.c/h` - Hardware abstraction layer (immutable per constraints)
- `comm_buffers.c/h` - Circular buffer implementation (RX/TX)
- `scpi_parser.c/h` - Command parsing logic
- `scheduler.c` - Cooperative multitasking scheduler

**Issues Identified:**
1. SCPI parsing (`scpi_process_line`) is called directly from communication task
2. No message buffer abstraction - commands go directly from RX buffer to parser
3. Static buffer in `task_comm.c` breaks task reentrancy
4. Response handling is inline rather than event-driven

## Proposed Architecture

### 1. Message Buffer Layer (`comm_message_buffer.h/c`)

New component to decouple RX raw data from command processing.

**State Machine: Message Extraction**
```c
typedef enum {
    MSG_STATE_WAITING_DELIMITER,  // Idle, waiting for message start
    MSG_STATE_RECEIVING,          // Accumulating bytes until delimiter
    MSG_STATE_READY,              // Complete message available
    MSG_STATE_ERROR               // Overflow or malformed message
} msg_state_t;

typedef struct {
    msg_state_t state;
    uint8_t data[MESSAGE_BUFFER_SIZE];
    size_t len;
} msg_message_t;
```

**API Functions:**
- `msg_buffer_init()` - Initialize message buffers
- `msg_extract_from_rx()` - Extract complete messages from RX buffer (called by comm task)
- `msg_get_next()` - Retrieve next message for processing (called by system task)
- `msg_mark_processed()` - Clear processed message
- `msg_is_response_pending()` - Check if SCPI generated responses

### 2. Refactored Communication Task (`task_comm.c`)

**Responsibility:** Only data ingestion and message extraction (Layer 1 + 2)

**State Machine:**
```
IDLE -> RX_ACTIVE [on start]
RX_ACTIVE -> MSG_EXTRACT -> TX_SEND -> IDLE [cycle]
TX_SEND -> RX_ACTIVE [on tx complete]
MSG_EXTRACT -> ERROR -> IDLE [on overflow]
```

**Key Changes:**
- Remove static `cmd_buffer` - use message buffer instead
- Only extract complete messages (newline delimited) to message buffer
- Notify system task via flag/event when messages available
- Handle TX transmission independently

### 3. Refactored System Task (`task_system.c`)

**Responsibility:** Command processing and response coordination (Layer 3 + 4)

**State Machine:**
```
IDLE -> PROCESS_CMD [when message available]
PROCESS_CMD -> NOTIFY_RESPONSE_READY [after SCPI processing]
NOTIFY_RESPONSE_READY -> IDLE [after notification]
```

**Key Changes:**
- Poll message buffer for new messages
- Call `scpi_process_line()` for each command
- Track when responses are ready via SCPI callback notification

### 4. Protocol Parser Enhancement (`scpi_parser.c`)

Modify to support response delimiter and terminator. Prepare for future SCPI parser library integration.

**Changes:**
- Add response delimiter (`;`) between commands
- Add terminator (`\n`) at end of response sequence
- Track response count for message completion

**API Addition:**
```c
typedef struct {
    bool (*is_message_complete)(void);
    void (*clear_response_state)(void);
} scpi_response_api_t;
```

## SCPI Parser Library v2 Compatibility Design

The architecture is designed to seamlessly integrate the SCPI parser library's core API:

### Library API Mapping

| Jaybee SCPI API | Current Implementation | Future Integration Point |
|-----------------|------------------------|--------------------------|
| `SCPI_Input()` | `scpi_process_message()` | Drop-in replacement for character/stream input |
| `SCPI_Parse()` | `scpi_process_line()` | Drop-in replacement for single command parsing |
| `SCPI_Init()` | Custom `scpi_init()` | Replace with library initialization |
| `SCPI_ErrorPush()` | `error_set()` | Map to project error handler |
| `SCPI_ErrorPop()` | `error_check()`/`error_clear()` | Map to project error handler |

### Integration Requirements

**Input Interface:**
The library's `SCPI_Input()` accepts chunks of input data with message end detection. Our architecture supports this by:
1. Feeding extracted messages (newline-delimited) to parser
2. Message extraction layer handles `\n` termination
3. No changes needed to buffer layer for integration

**Output Interface:**
The library uses callback-based results via `SCPI_ResultX()` functions. Our `scpi_output_interface_t` with `send_response()` callback is compatible.

**Error Queue:**
The library has `SCPI_ErrorPush()`/`SCPI_ErrorPop()` for error handling. Integration strategy:
1. Map `SCPI_ErrorPush(code, "message")` to `error_set(ERROR_QUERY_INTERRUPTED)` 
2. `-410` (Query Interrupted) is already defined in SCPI standard - library supports it natively
3. Error responses will be automatically generated by library

**Interface Structure Compatibility:**
The library uses `scpi_interface_t` with function pointers for I/O. Our design maps as follows:

```c
// Jaybee SCPI Parser scpi_interface_t structure (simplified)
typedef struct scpi_interface {
    uint32_t (*read)(scpi_t * context, char * data, uint32_t length);
    uint32_t (*write)(scpi_t * context, char * data, uint32_t length);
} scpi_interface_t;

// Our current scpi_output_interface_t will wrap this for compatibility
typedef struct {
    void (*send_response)(const char *resp, void *context);
    void *context;
} scpi_output_interface_t;
```

**Error Generator Handler:**
The library includes `SCPI_ErrorTranslate()` to convert error codes to strings. For `-410` Query Interrupted:
- Library returns standard format: "-410,\"Query INTERRUPTED\""
- Our implementation uses simplified format: "-410;" followed by actual response

### Future Migration Path

**Phase 1 (Current):** Internal `scpi_process_message()` wrapper
**Phase 2:** Replace internal parser with `#include <scpi_parser.h>` from library
**Phase 3:** Register commands using `scpi_command_t` structure
**Phase 4:** Remove deprecated `scpi_process_line()` function

### Command Registration Structure for Future Migration

The library uses `scpi_command_t` array. For future migration, commands will be declared as:
```c
static const scpi_command_t scpi_commands[] = {
    { .pattern = "*IDN?", .callback = SCPI_IdnQ, .tag = 0 },
    { .pattern = "MEAS:TEMP?", .callback = SCPI_MeasureTempQ, .tag = 0 },
    { .pattern = "TEMP:SP", .callback = SCPI_TempSp, .tag = 0 },
    // ... etc
};
```

## Implementation Files

| File | Changes |
|------|---------|
| `communication/comm_message_buffer.h/c` | NEW - Message extraction state machine |
| `communication/comm_buffers.h` | ADD `comm_buffer_tx_prepend()` function |
| `communication/comm_buffers.c` | IMPLEMENT prepend logic, add `comm_buffer_rx_clear()` |
| `communication/comm_interface.h` | ADD `COMM_STATE_RESPONSE_PENDING`, add getter/setter |
| `communication/comm_interface.c` | IMPLEMENT response pending state functions |
| `services/error_handler.h` | ADD `ERROR_QUERY_INTERRUPTED = 8` to enum |
| `services/scpi_parser.h` | ADD `scpi_process_message()` function |
| `services/scpi_parser.c` | IMPLEMENT multi-command parsing with query detection |
| `tasks/task_comm.c` | REFACTOR - Remove SCPI calls, add error injection |
| `tasks/task_system.c` | REFACTOR - Add message processing loop |

## Protocol Flow

```
Incoming Data: "MEAS:TEMP?;TEMP:SP?\n"
    ↓
RX Buffer (circular): [M][E][A][S]...[\n]
    ↓
Message Buffer: "MEAS:TEMP?;TEMP:SP?" (extracted)
    ↓
System Task processes:
    - "MEAS:TEMP?" → Response segment: "25.00;" (delimiter added)
    - "TEMP:SP?" → Response segment: "25.00" (final, no delimiter)
    ↓
Response Buffer: "25.00;25.00\r\n" [delimiter joins, terminator ends]
    ↓
Communication Task TX
```

### Multi-Response Response Building

The SCPI parser builds responses with proper delimiters:
- Each query response: `<value>;` (semicolon delimiter)
- Final response (or message without queries): `<value>\r\n` (terminator appended)
- Example: Two queries → "25.00;25.00\r\n"

## Error Handling: Query Interrupted (-410)

### Error Scenario
When a new query (command ending with `?`) is received before the previous message's
responses have been fully transmitted, the system must generate SCPI error -410 "Query INTERRUPTED".

### Critical Timing Analysis

The error condition is detected at **message extraction time** when:
- TX buffer has pending data (ongoing transmission), AND
- New incoming message contains at least one query command (`?`)

### Error Detection Logic

```
task_comm execution cycle:
├── Check TX buffer count > 0 → tx_in_progress = true
├── Extract complete message from RX buffer
├── If tx_in_progress AND message_has_query():
│   ├── Inject "-410;" error at TX buffer front
│   ├── Proceed with normal message processing
│   └── Responses will be: "-410;<resp1>;<resp2>\n"
└── Else:
    └── Normal processing path
```

### Response Pending State Management

The `COMM_STATE_RESPONSE_PENDING` flag tracks whether the TX buffer contains responses from a message that included queries:

**Set by:** System task after `scpi_process_message()` generates responses for query commands

**Cleared by:** Communication task when:
1. TX buffer count reaches zero (all data transmitted), OR
2. New query interrupted condition occurs (new error injected)

**Error injection condition:** New message extracted contains query (`?`) WHILE previous response data still pending in TX buffer

### Implementation Changes

| Component | Changes |
|-----------|---------|
| `comm_interface.h` | Add `COMM_STATE_RESPONSE_PENDING` state flag |
| `comm_buffers.c` | Add `error_set(ERROR_QUERY_INTERRUPTED)` call |
| `task_comm.c` | Add query detection, error injection logic |
| `task_system.c` | Clear response flag after processing complete |
| `scpi_parser.c` | Generate "-410" error string on interrupt |
| `error_handler.h` | Add `ERROR_QUERY_INTERRUPTED = 8` to error_code_t enum |

## Protocol Flow with Error Handling

```
Normal Flow:
Message 1: "MEAS:TEMP?\n" → Response: "25.00\r\n" [queued in TX buffer, tx_pending=true]
TX completes → tx_pending=false

Query Interrupted Flow:
Message 1: "MEAS:TEMP?\n" → Response: "25.00\r\n" [queued in TX buffer, tx_pending=true]
Message 2: "TEMP:SP?\n" [extracted while tx_pending=true AND contains query]
    → Inject "-410;" at TX buffer start
    → Process Message 2: Response: "25.00\r\n"
    ↓
TX Buffer: "-410;25.00\r\n" transmitted atomically
```

### Error Detection Timing Sequence

```
Time T0: Message A (query) received → parsed → responses queued in TX buffer
Time T1: TX transmission starts, TX buffer has data, response_pending = true
Time T2: Message B arrives in RX buffer
Time T3: task_comm extracts Message B
Time T4: Error check: TX buffer NOT empty + Message B has query → inject "-410;"
Time T5: task_system processes Message B normally
Time T6: TX buffer now contains: "-410;" + "response_B\r\n"
Time T7: Combined response transmitted: "-410;response_B\r\n"
```

### Error Injection vs Response Generation Order

The error must be injected BEFORE the new message's responses are generated to ensure proper ordering:
1. Extract message from RX buffer
2. Check query interrupted condition
3. If interrupted, inject error
4. Process message (generate responses)
5. TX buffer contains complete response sequence

```
Response Assembly:
1. Responses accumulated in TX buffer as they are generated
2. Query detection: scan extracted message for `?` before processing
3. If TX pending AND new query detected:
   - Prepend "-410;" to TX buffer (front-insert)
   - Continue with normal response generation
   - Final response: "-410;<resp1>;<resp2>\r\n"
4. Clear pending state when TX buffer empties
```

## Detailed State Machine Implementations

### Communication Task State Machine (task_comm.c)

```c
typedef enum {
    COMM_STATE_IDLE,
    COMM_STATE_MSG_EXTRACT,
    COMM_STATE_TX_CHECK
} comm_task_state_t;

// State transition table
void task_comm(void) {
    static comm_task_state_t state = COMM_STATE_IDLE;
    
    switch (state) {
        case COMM_STATE_IDLE:
            // Check for response pending flag from system task
            if (comm_interface_is_response_pending(COMM_IFACE_USART)) {
                comm_interface_set_response_pending(COMM_IFACE_USART, false);
            }
            // Check if messages available in RX buffer
            if (comm_buffer_rx_count(COMM_IFACE_USART) > 0) {
                state = COMM_STATE_MSG_EXTRACT;
            } else if (comm_buffer_tx_count(COMM_IFACE_USART) > 0) {
                state = COMM_STATE_TX_CHECK;
            }
            break;
            
        case COMM_STATE_MSG_EXTRACT:
            // Step 1: Extract complete message from RX buffer
            msg_extract_from_rx();
            
            // Step 2: Check for query interrupted condition
            // If TX buffer has data AND extracted message contains query
            if (comm_buffer_tx_count(COMM_IFACE_USART) > 0 && msg_contains_query()) {
                // Inject "-410;" error at front of TX buffer BEFORE processing
                comm_buffer_tx_prepend(COMM_IFACE_USART, (const uint8_t*)"-410;", 4);
                error_set(ERROR_QUERY_INTERRUPTED);
            }
            
            // Step 3: Signal system task to process the message
            signal_system_task();
            state = COMM_STATE_TX_CHECK;
            break;
            
        case COMM_STATE_TX_CHECK:
            if (comm_buffer_tx_count(COMM_IFACE_USART) > 0 && 
                comm_interface_is_tx_ready(COMM_IFACE_USART)) {
                comm_interface_set_tx_busy(COMM_IFACE_USART, true);
                comm_interface_start_tx(COMM_IFACE_USART);
            }
            state = COMM_STATE_IDLE;
            break;
    }
}
```

**Note:** The error injection happens AFTER message extraction but BEFORE `signal_system_task()`. This ensures the error is in the TX buffer before the system task generates new responses.

### System Task State Machine (task_system.c)

```c
typedef enum {
    SYS_STATE_IDLE,
    SYS_STATE_PROCESS_MSG
} sys_task_state_t;

void task_system(void) {
    static sys_task_state_t state = SYS_STATE_IDLE;
    
    if (state == SYS_STATE_IDLE && is_msg_ready_flag()) {
        state = SYS_STATE_PROCESS_MSG;
    }
    
    if (state == SYS_STATE_PROCESS_MSG) {
        const char *msg = msg_get_next();
        if (msg) {
            scpi_process_message(msg);  // Parses all commands, generates responses
            msg_mark_processed();
        }
        state = SYS_STATE_IDLE;
    }
}
```

### Message Processor State Machine (embedded in scpi_parser)

```c
typedef enum {
    PARSER_STATE_WAIT_CMD,
    PARSER_STATE_PROCESSING,
    PARSER_STATE_RESP_READY
} parser_state_t;

static parser_state_t parser_state = PARSER_STATE_WAIT_CMD;
static size_t response_count = 0;
static bool message_had_query = false;

void scpi_process_message(const char *message) {
    char cmd[MAX_CMD_LEN];
    size_t cmd_len = 0;
    response_count = 0;
    message_had_query = false;
    
    // Split by command delimiter (';' or end of message)
    for (size_t i = 0; message[i] != '\0' && message[i] != '\n'; i++) {
        if (message[i] == ';') {
            cmd[cmd_len] = '\0';
            if (cmd_len > 0) {
                if (strchr(cmd, '?') != NULL) {
                    message_had_query = true;
                }
                scpi_process_line(cmd);
                response_count++;
            }
            cmd_len = 0;
        } else {
            if (cmd_len < MAX_CMD_LEN - 1) {
                cmd[cmd_len++] = message[i];
            }
        }
    }
    
    // Process final command if no trailing ';'
    if (cmd_len > 0) {
        cmd[cmd_len] = '\0';
        if (strchr(cmd, '?') != NULL) {
            message_had_query = true;
        }
        scpi_process_line(cmd);
        response_count++;
    }
    
    // Signal that response processing is complete for this message
    if (message_had_query && response_count > 0) {
        // Don't clear here - let comm task clear after TX
    }
}
```

## Key Design Decisions

1. **No modification to `usart_hw.c`** - All hardware interaction through existing API
2. **Message buffer size: 256 bytes** - Matches existing buffer sizes
3. **Command delimiter: `;` or `\n`** - Split commands on semicolon within message
4. **Response delimiter: `;`** - Join multiple responses with semicolon
5. **Event notification via flag** - Simple boolean flag to signal between tasks
6. **Error prepend mechanism** - `comm_buffer_tx_prepend()` inserts error at TX buffer front
7. **Thread-safety** - All buffer operations use protect/unprotect from comm_interface

## Memory Layout

```
RX Buffer (256 bytes): Raw bytes from USART idle interrupt
Message Buffer (256 bytes): Complete message extracted (newline-delimited)
TX Buffer (256 bytes): Response strings ready for transmission
```

## API Extensions Required

### comm_interface.h
```c
void comm_interface_set_response_pending(comm_interface_id_t id, bool pending);
bool comm_interface_is_response_pending(comm_interface_id_t id);
```

### comm_buffers.h
```c
bool comm_buffer_tx_prepend(comm_interface_id_t iface_id, const uint8_t *data, size_t len);
```

### scpi_parser.h
```c
void scpi_process_message(const char *message);  // Multi-command version
```

## Testing Strategy

### Normal Operation Tests
1. Unit test message extraction with various delimiter combinations
2. Test multi-command messages ("CMD1;CMD2;CMD3\n")
3. Verify response concatenation works correctly
4. Test error conditions (buffer overflow, malformed messages)

### Query Interrupted Tests
5. **Query Interrupted Scenario**: Send query, before response TX complete send another query → expect "-410" error
6. **Query Interrupted Timing**: Vary timing between first response generation and second query arrival
7. **Mixed Commands**: Send "CMD1?;CMD2" before TX complete → expect "-410" before CMD2 response
8. **No Query in Second Message**: Send "MEAS:TEMP?\n", then "TEMP:SP 30\n" before TX → no error, normal response
9. **Error Response Format**: Verify "-410;" delimiter followed by terminator "\r\n"
10. **Multiple Interruptions**: Cascading interrupted queries handled correctly

### Error Recovery Tests
11. Verify system recovers after -410 error, continues processing subsequent queries
12. TX buffer overflow during error response handling

## Scalability Considerations

### Multi-Interface Support
The architecture supports multiple communication interfaces (TCP, USB) by:
- Using `comm_interface_id_t` enum for interface selection
- Interface-agnostic message buffer with per-interface indexing
- Task-level state maintained per-interface

### Buffer Management Improvements
Future enhancements can include:
- Configurable buffer sizes via compile-time defines
- Dynamic buffer allocation for high-load scenarios
- Priority queuing for error vs. response messages

### Library Integration Robustness
The design maintains loose coupling with the SCPI parser through:
- Abstract input/output interfaces
- Error handling abstraction layer
- Non-blocking message processing model