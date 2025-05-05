# Deribit Trading API Documentation

## Authentication
All private API routes require authentication using a token obtained from the `/api/auth` endpoint.

## Public API Routes

### 1. Health Check
```http
GET /api/health
```
- **Response:** JSON containing service status
```json
{
  "status": "ok",
  "service": "Crow Deribit Trading API",
  "uptime": "running"
}
```

### 2. Authentication
```http
POST /api/auth
```
- **Body:**
```json
{
  "client_id": "string",
  "client_secret": "string"
}
```
- **Response:** JSON containing access token
```json
{
  "access_token": "string",
  "http_status": 200,
  "latency_ms": number
}
```

### 3. Get Order Book
```http
GET /api/orderbook
```
- **Query Parameters:**
  - `instrument` (required): Trading pair symbol (e.g., "BTC-PERPETUAL")
  - `depth` (optional): Number of order book levels (default: 2)
- **Response:** JSON containing order book data

## Private API Routes

### 1. Place Buy Order
```http
POST /api/buy
```
- **Headers:**
  - `Authorization: Bearer {token}`
- **Body:**
```json
{
  "instrument": "string",
  "amount": number,
  "type": "market" | "limit",
  "price": number  // Required only for limit orders
}
```
- **Response:** JSON containing order details

### 2. Cancel Order
```http
DELETE /api/cancel
```
- **Headers:**
  - `Authorization: Bearer {token}`
- **Query Parameters:**
  - `order_id` (required): ID of the order to cancel
- **Response:** JSON containing cancellation status

### 3. Get Open Orders
```http
GET /api/orders
```
- **Headers:**
  - `Authorization: Bearer {token}`
- **Query Parameters:**
  - `instrument` (required): Trading pair symbol
- **Response:** JSON containing list of open orders

### 4. Get Positions
```http
GET /api/positions
```
- **Headers:**
  - `Authorization: Bearer {token}`
- **Query Parameters:**
  - `currency` (required): Currency to get positions for
- **Response:** JSON containing current positions

### 5. Modify Order
```http
PUT /api/modify
```
- **Headers:**
  - `Authorization: Bearer {token}`
- **Body:**
```json
{
  "order_id": "string",
  "amount": number,
  "price": number
}
```
- **Response:** JSON containing modified order details

## WebSocket API Routes

### 1. Place Buy Order (WebSocket)
```http
POST /api/ws/buy
```
- **Body:**
```json
{
  "symbol": "string",
  "amount": number,
  "type": "market" | "limit",
  "price": number  // Required only for limit orders
}
```
- **Response:** JSON containing order details

### 2. Get Open Orders (WebSocket)
```http
GET /api/ws/open_orders
```
- **Query Parameters:**
  - `instrument` (optional): Trading pair symbol (default: "BTC-PERPETUAL")
- **Response:** JSON containing list of open orders

### 3. Cancel Order (WebSocket)
```http
DELETE /api/ws/cancel_order
```
- **Query Parameters:**
  - `order_id` (required): ID of the order to cancel
- **Response:** JSON containing cancellation status

### 4. Modify Order (WebSocket)
```http
PATCH /api/ws/modify_order
```
- **Body:**
```json
{
  "order_id": "string",
  "price": number,
  "amount": number
}
```
- **Response:** JSON containing modified order details

### 5. Get Positions (WebSocket)
```http
GET /api/ws/positions
```
- **Query Parameters:**
  - `currency` (optional): Currency to get positions for (default: "BTC")
- **Response:** JSON containing current positions

### 6. Get Order Book (WebSocket)
```http
GET /api/ws/orderbook
```
- **Query Parameters:**
  - `instrument` (optional): Trading pair symbol (default: "BTC-PERPETUAL")
  - `depth` (optional): Number of order book levels (default: 1)
- **Response:** JSON containing order book data

## Error Responses
All API routes may return the following error responses:

```json
{
  "error": {
    "code": number,
    "message": "string"
  }
}
```

Common error codes:
- 400: Bad Request (Invalid parameters)
- 401: Unauthorized (Missing or invalid token)
- 500: Internal Server Error
- 504: Gateway Timeout (Timeout waiting for Deribit response)

## Notes
1. All WebSocket routes use the same WebSocket connection to Deribit
2. WebSocket responses include latency information in the `meta` field
3. All private routes require a valid Bearer token in the Authorization header
4. The API server runs on port 8080 by default
5. All responses are in JSON format
6. The API supports both HTTP and WebSocket connections to Deribit
7. WebSocket routes have a timeout of 5 seconds for responses
8. All routes are logged with detailed information including latency and response codes check them on folder /build/log
