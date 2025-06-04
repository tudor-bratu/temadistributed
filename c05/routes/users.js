// routes/users.js
const express = require('express');
const router = express.Router();
const { pool } = require('../db'); // Import the database pool

// GET all users
router.get('/', async (req, res) => {
  try {
    const [rows] = await pool.execute('SELECT * FROM users');
    res.json(rows);
  } catch (error) {
    console.error('Error fetching users:', error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// GET user by ID
router.get('/:id', async (req, res) => {
  const { id } = req.params;
  try {
    const [rows] = await pool.execute('SELECT * FROM users WHERE id = ?', [id]);
    if (rows.length === 0) {
      return res.status(404).json({ error: 'User not found' });
    }
    res.json(rows[0]);
  } catch (error) {
    console.error(`Error fetching user with ID ${id}:`, error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// POST new user
router.post('/', async (req, res) => {
  const { name, email } = req.body;
  if (!name || !email) {
    return res.status(400).json({ error: 'Name and email are required' });
  }
  try {
    const [result] = await pool.execute('INSERT INTO users (name, email) VALUES (?, ?)', [name, email]);
    res.status(201).json({ id: result.insertId, name, email });
  } catch (error) {
    console.error('Error creating user:', error);
    // Check for duplicate email error (MySQL error code 1062)
    if (error.code === 'ER_DUP_ENTRY') {
      return res.status(409).json({ error: 'Email already exists' });
    }
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// PUT update user by ID
router.put('/:id', async (req, res) => {
  const { id } = req.params;
  const { name, email } = req.body;
  if (!name || !email) {
    return res.status(400).json({ error: 'Name and email are required' });
  }
  try {
    const [result] = await pool.execute('UPDATE users SET name = ?, email = ? WHERE id = ?', [name, email, id]);
    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'User not found' });
    }
    res.json({ message: 'User updated successfully', id, name, email });
  } catch (error) {
    console.error(`Error updating user with ID ${id}:`, error);
    if (error.code === 'ER_DUP_ENTRY') {
      return res.status(409).json({ error: 'Email already exists' });
    }
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// DELETE user by ID

router.delete('/:id', async (req, res) => {
  const { id } = req.params;
  try {
    const [result] = await pool.execute('DELETE FROM users WHERE id = ?', [id]);
    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'User not found' });
    }
    res.json({ message: 'User deleted successfully', id });
  } catch (error) {
    console.error(`Error deleting user with ID ${id}:`, error);
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

module.exports = router;