<?php
        echo "HTTP " . $_SERVER['REQUEST_METHOD'] . ': ';
if ($_SERVER['REQUEST_METHOD'] == 'GET') {
        echo $_GET['data'];
} else if ($_SERVER['REQUEST_METHOD'] == 'POST' || $_SERVER['REQUEST_METHOD'] = 'PUT') {
        echo file_get_contents('php://input');
} else if ($_SERVER['REQUEST_METHOD'] == 'DELETE') {
        echo "DELETE. OK.";
} else {
        echo "Unknown method type " . $_SERVER['REQUEST_METHOD'];
}
?>
