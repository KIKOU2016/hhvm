<?hh
$obj = new ArrayObject(array(1,2));
var_dump(count($obj));
class ArrayObject2 extends ArrayObject {
    public function count() {
        return -parent::count();
    }
}
$obj = new ArrayObject2(array(1,2));
var_dump(count($obj));
