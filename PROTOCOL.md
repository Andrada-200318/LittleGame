# Network protocol (v2)

This file is the contract between the C++ server and the Kotlin client. Both sides implement exactly
what is written here. To change something, both devs agree first, then this file is edited in its
own commit, then the code changes on both sides.

## 1. Transport

- TCP. The server listens on `0.0.0.0` (all interfaces), port **5000**.
- One TCP connection per player, kept open the whole time the player is in the room.
- Text, **UTF-8**. One message = one line ending in `\n`. Receivers also accept `\r\n` (strip a trailing `\r`).
- Max line length: **2048 bytes**. The server disconnects a client that sends a longer line.
- TCP is a stream: one read can return half a line or several lines. Always buffer until `\n`.

## 2. Message format

```
COMMAND|field1|field2|...
```

- Fields are separated by the pipe character `|`. Commands are uppercase.
- Every command has a fixed number of fields (listed below). Split into at most fieldCount + 1 parts
  (the command plus its fields): **the last field takes the rest of the line** and may itself contain `|`.
  Kotlin: `line.split("|", limit = fieldCount + 1)`. C++: split only on the first `fieldCount` pipes.
- No field may contain `\n` or `\r`. The client replaces them with a space in user-typed text.
- `roundId` is a decimal integer. The first round of a room is 1; it goes up by 1 every round.

## 3. Limits and constants

| Name | Value | Rule |
|---|---|---|
| PORT | 5000 | |
| MIN_PLAYERS | 3 | needed to start a round |
| MAX_PLAYERS | 8 | the 9th join gets `ROOM_FULL` |
| Username | 1–16 characters | trimmed; no pipe or comma; unique in the room, case-insensitive |
| Password | 1–20 characters | no pipe; case-sensitive; set by the host |
| Answer | 1–200 characters | trimmed; not empty |
| EARLY_STOP_SECONDS | 5 | server-side only; the phones aren't told |
| JOIN_TIMEOUT | 10 s | server closes a connection that sends no `JOIN` within 10 s (milestone C) |

The client enforces character limits in the UI. The server checks bytes as a safety net, since one
UTF-8 character can be up to 4 bytes: username ≤ 64 bytes, answer ≤ 800 bytes.

## 4. Client → server

```
JOIN|<password>|<username>      2 fields. First message after connecting, exactly once.
ANSWER|<roundId>|<text>         2 fields. After ROUND_START, at most once per round.
LEAVE                           0 fields. Player pressed Exit room; the client closes the socket right after.
```

## 5. Server → client

```
JOIN_OK|<username>|<status>                 2 fields. Joined. username = the name as stored (trimmed).
                                            status = LOBBY (no round running)
                                                  or WAIT_NEXT_ROUND (a round is running; you play from the next one).
JOIN_ERROR|<code>|<message>                 2 fields. Join refused. The server closes the connection right after.
ROUND_START|<roundId>|<role>|<question>     3 fields. New round. role = NORMAL or IMPOSTOR.
ANSWER_OK|<roundId>                         1 field.  Answer accepted.
ANSWER_REJECTED|<roundId>|<code>            2 fields. Answer refused (codes below).
ANSWERS_CLOSED|<roundId>                    1 field.  No more answers are accepted for this round.
ROOM_CLOSED|<message>                       1 field.  Host ended the room. The server closes the connection right after.
ERROR|<code>|<message>                      2 fields. The client broke the protocol (a bug).
```

`<message>` is English text for logs. The client decides what to show based on `<code>`.

## 6. Codes

**`JOIN_ERROR`**, checked in this order:

| Code | Cause |
|---|---|
| `WRONG_PASSWORD` | Password doesn't match. |
| `INVALID_NAME` | Empty after trimming, too long, or contains a pipe or a comma. |
| `NAME_TAKEN` | Someone in the room already has this name (case-insensitive). |
| `ROOM_FULL` | The room already has MAX_PLAYERS players. |

**`ANSWER_REJECTED`**:

| Code | Cause |
|---|---|
| `WRONG_ROUND` | `roundId` isn't the current round. |
| `NOT_IN_ROUND` | The player joined after this round started. |
| `ALREADY_ANSWERED` | A second answer in the same round. |
| `ANSWERS_CLOSED` | Answers are already closed. |
| `INVALID_ANSWER` | Empty after trimming, or too long. |

**`ERROR`**:

| Code | Cause | Server then |
|---|---|---|
| `NOT_JOINED` | First message wasn't `JOIN`. | closes the connection |
| `LINE_TOO_LONG` | Line over 2048 bytes. | closes the connection |
| `ALREADY_JOINED` | `JOIN` sent twice. | ignores it |
| `BAD_FORMAT` | Wrong number of fields, or `roundId` isn't a number. | ignores it |
| `UNKNOWN_COMMAND` | Command not in section 4. | ignores it |

## 7. Rules

**Server**
- `ROUND_START` goes only to players who were in the room when the host pressed Start round or New
  round (the round's *participants*). Exactly one participant gets `IMPOSTOR` and a different question;
  all others get the same question.
- A player who joins during a round gets `JOIN_OK|<name>|WAIT_NEXT_ROUND` and nothing else until the
  next `ROUND_START`.
- Answers are accepted from `ROUND_START` until `ANSWERS_CLOSED`, including during the early-stop countdown.
- The server sends `ANSWERS_CLOSED` to every connected participant as soon as **either** every
  participant who is still connected has answered **or** the early-stop countdown ends.
- Early stop is not announced to the phones. When its 5 seconds run out, they just get `ANSWERS_CLOSED`.
- The server never sends anyone else's answers, names, roles or questions to a phone. The reveal
  happens only on the host screen.
- A player who sends `LEAVE` or drops is removed from the room. If a round is running, they stop
  blocking it, and their answer (if any) is still revealed on the host screen.

**Client**
- Sends `JOIN` right after connecting. If neither `JOIN_OK` nor `JOIN_ERROR` arrives within 5 s,
  closes the connection and shows "The host didn't answer".
- Sends at most one `ANSWER` per `roundId`, only between `ROUND_START` and `ANSWERS_CLOSED`.
- Ignores `ANSWER_OK`, `ANSWER_REJECTED` and `ANSWERS_CLOSED` whose `roundId` isn't the current round.
- Treats the socket closing without `ROOM_CLOSED` as "Lost connection to the host".
- Logs and ignores lines it doesn't understand. It must never crash on bad input.

## 8. Example session

Ana, Bob and Dan play; Bob is the impostor; Eve joins mid-round.

```
Ana → JOIN|pizza42|Ana                        ← JOIN_OK|Ana|LOBBY
Bob → JOIN|pizza42|Bob                        ← JOIN_OK|Bob|LOBBY
Dan → JOIN|wrong|Dan                          ← JOIN_ERROR|WRONG_PASSWORD|Wrong password
      (server closes Dan's socket; Dan reconnects)
Dan → JOIN|pizza42|Dan                        ← JOIN_OK|Dan|LOBBY

[host presses Start round]
Ana ← ROUND_START|1|NORMAL|What's your favorite food?
Bob ← ROUND_START|1|IMPOSTOR|What's your favorite dessert?
Dan ← ROUND_START|1|NORMAL|What's your favorite food?

Ana → ANSWER|1|Sarmale, obviously             ← ANSWER_OK|1
Eve → JOIN|pizza42|Eve                        ← JOIN_OK|Eve|WAIT_NEXT_ROUND
Bob → ANSWER|1|Tiramisu                       ← ANSWER_OK|1

[host presses Early stop; the phones aren't told]
[5 seconds later; Dan never answered]
Ana, Bob, Dan ← ANSWERS_CLOSED|1
Dan → ANSWER|1|Pasta                          ← ANSWER_REJECTED|1|ANSWERS_CLOSED   (pressed Send too late)

[host reveals answers and the impostor on the host screen; phones receive nothing]
[host presses New round → Ana, Bob, Dan and Eve get ROUND_START|2|...]

Bob → LEAVE                                   (Bob's socket closes)
[host presses End room]
Ana, Dan, Eve ← ROOM_CLOSED|Host ended the room
```

## Changelog

- v1 (2026-11-02): first version.
- v2 (2026-11-02): removed `EARLY_STOP`; early stop happens only on the server.
