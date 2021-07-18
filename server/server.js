const express = require("express");
const cors = require("cors");
const dotenv = require("dotenv");

// setup secrets from dotenv
require("dotenv").config();

// express initialization
const app = express();

//routes

// middleware

// starting server 
const PORT = process.env.PORT;
const URI = process.env.pg_URI;
app.listen(PORT, () => {
    console.log(`Server running on port ${PORT}`);
});

const { Client } = require('pg');

const client = new Client({
  connectionString: process.env.pg_URI,
  ssl: {
    rejectUnauthorized: false
  }
});

client.connect();

client.query('SELECT table_schema,table_name FROM information_schema.tables;', (err, res) => {
  if (err) throw err;
  for (let row of res.rows) {
    console.log(JSON.stringify(row));
  }
  client.end();
});

// connection to Postgress Database 
