<?hh
/**
 * Copyright (c) 2014, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 */

/**
 * This file provides type information for some of HHVM's builtin classes.
 *
 * YOU SHOULD NEVER INCLUDE THIS FILE ANYWHERE!!!
 */

/**
 * `Set` is an ordered set-style collection. HHVM provides a native
 * implementation for this class. The PHP class definition below is not
 * actually used at run time; it is simply provided for the typechecker and
 * for developer reference.
 *
 * Like all objects in PHP, `Set`s have reference-like semantics. When a caller
 * passes a `Set` to a callee, the callee can modify the `Set` and the caller
 * will see the changes. `Set`s do not have "copy-on-write" semantics.
 *
 * `Set`s preserve insertion order of the elements. When iterating over a
 * `Set`, the elements appear in the order they were inserted. Also, `Set`s do
 * not automagically convert integer-like strings (ex. "123") into integers.
 *
 * `Set`s only support `int` values and `string` values. If a value of a
 * different type is used, an exception will be thrown.
 *
 * In general, Sets do not support `$c[$k]` style syntax. Adding an element
 * using `$c[] = ..` syntax is supported.
 *
 * `Set` do not support iteration while elements are being added or removed.
 * When an element is added or removed, all iterators that point to the `Set`
 * shall be considered invalid.
 *
 * `Set`s do not support taking elements by reference. If binding assignment
 * (`=&`) is used when adding a new element to a `Set` (ex. `$c[] =& ...`), or
 * if a `Set` is used with `foreach` by reference, an exception will be thrown.
 *
 * @guide /hack/collections/introduction
 * @guide /hack/collections/classes
 */

final class Set<Tv as arraykey> implements MutableSet<Tv> {
  /**
   * Creates a `Set` from the given `Traversable`, or an empty `Set` if `null`
   * is passed.
   *
   * @param $it - any `Traversable` object from which to create the `Set`
   *              (e.g., `array`). If `null`, then an empty `Set` is created.
   */
  <<__Rx, __AtMostRxAsArgs>>
  public function __construct(<<__MaybeMutable, __OnlyRxIfImpl(HH\Rx\Traversable::class)>> ?Traversable<Tv> $it);

  /**
   * Returns an `array` containing the values from the current `Set`.
   *
   * For the returned `array`, each key is the same as its associated value.
   *
   * @return - an `array` containing the values from the current `Set`, where
   *           each key of the `array` are the same as each value.
   */
  <<__Rx, __MaybeMutable, __PHPStdLib>>
  public function toArray(): array<arraykey, Tv>;

  /**
   * Returns an `array` containing the values from the current `Set`.
   *
   * `Set`s don't have keys. So this method just returns the values.
   *
   * This method is interchangeable with `toValuesArray()`.
   *
   * @return - an integer-indexed `array` containing the values from the
   *           current `Set`.
   */
  <<__Rx, __MaybeMutable>>
  public function toKeysArray(): varray<Tv>;

  /**
   * Returns an `array` containing the values from the current `Set`.
   *
   * This method is interchangeable with `toKeysArray()`.
   *
   * @return - an integer-indexed `array` containing the values from the
   *           current `Set`.
   */
  <<__Rx, __MaybeMutable>>
  public function toValuesArray(): varray<Tv>;

  /**
   * Returns a `Vector` of the current `Set` values.
   *
   * @return - a `Vector` (integer-indexed) that contains the values of the
   *           current `Set`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function toVector(): Vector<Tv>;

  /**
   * Returns an immutable vector (`ImmVector`) with the values of the current
   * `Set`.
   *
   * @return - an `ImmVector` (integer-indexed) with the values of the current
   *           `Set`.
   */
  <<__Rx, __MaybeMutable>>
  public function toImmVector(): ImmVector<Tv>;

  /**
   * Returns a `Map` based on the values of the current `Set`.
   *
   * Each key of the `Map` will be the same as its value.
   *
   * @return - a `Map` that that contains the values of the current `Set`, with
   *           each key of the `Map` being the same as its value.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function toMap(): Map<arraykey, Tv>;

  /**
   * Returns an immutable map (`ImmMap`) based on the values of the current
   * `Set`.
   *
   * Each key of the `Map` will be the same as its value.
   *
   * @return - an `ImmMap` that that contains the values of the current `Set`,
   *           with each key of the Map being the same as its value.
   */
  <<__Rx, __MaybeMutable>>
  public function toImmMap(): ImmMap<arraykey, Tv>;

  /**
   * Returns a deep copy of the current `Set`.
   *
   * @return - a `Set` that is a deep copy of the current `Set`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function toSet(): Set<Tv>;

  /**
   * Returns an immutable (`ImmSet`), deep copy of the current `Set`.
   *
   * This method is interchangeable with `immutable()`.
   *
   * @return - an `ImmSet` that is a deep copy of the current `Set`.
   */
  <<__Rx, __MaybeMutable>>
  public function toImmSet(): ImmSet<Tv>;

  /**
   * Returns an immutable (`ImmSet`), deep copy of the current `Set`.
   *
   * This method is interchangeable with `toImmSet()`.
   *
   * @return - an `ImmSet` that is a deep copy of the current `Set`.
   */
  <<__Rx, __MaybeMutable>>
  public function immutable(): ImmSet<Tv>;

  /**
   * Returns a lazy, access elements only when needed view of the current
   * `Set`.
   *
   * Normally, memory is allocated for all of the elements of the `Set`. With
   * a lazy view, memory is allocated for an element only when needed or used
   * in a calculation like in `map()` or `filter()`.
   *
   * @return - an `KeyedIterable` representing the lazy view into the current
   *           `Set`, where the keys are the same as the values.
   *
   * @guide /hack/collections/examples
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function lazy(): HH\Rx\KeyedIterable<arraykey, Tv>;

  /**
   * Returns a `Vector` containing the values of the current `Set`.
   *
   * This method is interchangeable with `toVector()` and `keys()`.
   *
   * @return - a `Vector` (integer-indexed) containing the values of the
   *           current `Set`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function values(): Vector<Tv>;

  /**
   * Returns a `Vector` containing the values of the current `Set`.
   *
   * `Set`s don't have keys, so this will return the values.
   *
   * This method is interchangeable with `toVector()` and `values()`.
   *
   * @return - a `Vector` (integer-indexed) containing the values of the
   *           current `Set`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function keys(): Vector<arraykey>;

  /**
   * Returns a `Set` containing the values after an operation has been applied
   * to each value in the current `Set`.
   *
   * Every value in the current `Set` is affected by a call to `map()`, unlike
   * `filter()` where only values that meet a certain criteria are affected.
   *
   * @param $callback - The callback containing the operation to apply to the
   *                    current `Set` values.
   *
   * @return - a `Set` containing the values after a user-specified operation
   *           is applied.
   *
   * @guide /hack/collections/examples
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function map<Tu as arraykey>(<<__AtMostRxAsFunc>>(function(Tv): Tu) $callback): Set<Tu>;

  /**
   * Returns a `Set` containing the values after an operation has been applied
   * to each "key" and value in the current `Set`.
   *
   * Since `Set`s don't have keys, the callback uses the values as the keys
   * as well.
   *
   * Every value in the current `Set` is affected by a call to `mapWithKey()`,
   * unlike `filterWithKey()` where only values that meet a certain criteria are
   * affected.
   *
   * @param $callback - The callback containing the operation to apply to the
   *                    current `Set` keys and values.
   *
   * @return - a `Set` containing the values after a user-specified operation
   *           on the current `Set`'s values is applied.
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function mapWithKey<Tu as arraykey>(<<__AtMostRxAsFunc>>(function(arraykey, Tv): Tu) $callback): Set<Tu>;

  /**
   * Returns a `Set` containing the values of the current `Set` that meet
   * a supplied condition applied to each value.
   *
   * Only values that meet a certain criteria are affected by a call to
   * `filter()`, while all values are affected by a call to `map()`.
   *
   * @param $callback - The callback containing the condition to apply to the
   *                    current `Set` values.
   *
   * @return - a `Set` containing the values after a user-specified condition
   *           is applied.
   *
   * @guide /hack/collections/examples
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function filter(<<__AtMostRxAsFunc>>(function(Tv): bool) $callback): Set<Tv>;

  /**
   * Returns a `Set` containing the values of the current `Set` that meet
   * a supplied condition applied to its "keys" and values.
   *
   * Since `Set`s don't have keys, the callback uses the values as the keys
   * as well.
   *
   * Only values that meet a certain criteria are affected by a call to
   * `filterWithKey()`, while all values are affected by a call to
   * `mapWithKey()`.
   *
   * @param $callback - The callback containing the condition to apply to the
   *                    current `Set` keys and values.
   *
   * @return - a `Set` containing the values after a user-specified condition
   *           is applied to the values of the current `Set`.
   *
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function filterWithKey(<<__AtMostRxAsFunc>>(function(arraykey, Tv): bool) $callback): Set<Tv>;

  /**
   * Alters the current `Set` so that it only contains the values that meet a
   * supplied condition on each value.
   *
   * This method is like `filter()`, but mutates the current `Set` too in
   * addition to returning the current `Set`.
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $callback - The callback containing the condition to apply to the
   *                    current `Set` values.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __AtMostRxAsArgs, __ReturnsVoidToRx>>
  public function retain(<<__AtMostRxAsFunc>>(function(Tv): bool) $callback): Set<Tv>;

  /**
   * Alters the current `Set` so that it only contains the values that meet a
   * supplied condition on its "keys" and values.
   *
   * `Set`s don't have keys, so the `Set` values are used as the key in the
   * callback.
   *
   * This method is like `filterWithKey()`, but mutates the current `Set` too
   * in addition to returning the current `Set`.
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $callback - The callback containing the condition to apply to the
   *                    current `Set` values.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __AtMostRxAsArgs, __ReturnsVoidToRx>>
  public function retainWithKey(<<__AtMostRxAsFunc>>(function(arraykey, Tv): bool) $callback): Set<Tv>;

  /**
   * Throws an exception unless the current `Set` or the `Traversable` is
   * empty.
   *
   * Since `Set`s only support integers or strings as values, we cannot have
   * a `Pair` as a `Set` value. So in order to avoid an
   * `InvalidArgumentException`, either the current `Set` or the `Traversable`
   * must be empty so that we actually return an empty `Set`.
   *
   * @param $traversable - The `Traversable` to use to combine with the
   *                       elements of the current `Set`.
   *
   * @return - The `Set` that combines the values of the current `Set` with
   *           the provided `Traversable`; one of these must be empty or an
   *           exception is thrown.
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function zip<Tu>(
    <<__MaybeMutable, __OnlyRxIfImpl(HH\Rx\Traversable::class)>> Traversable<Tu> $traversable
  /* HH_FIXME[4110] need bottom type as generic */
  /* HH_FIXME[4322] */
  ): Set<Pair<Tv, Tu>>;

  /**
   * Returns a `Set` containing the first `n` values of the current `Set`.
   *
   * The returned `Set` will always be a proper subset of the current `Set`.
   *
   * `n` is 1-based. So the first element is 1, the second 2, etc.
   *
   * @param $n - The last element that will be included in the `Set`.
   *
   * @return - A `Set` that is a proper subset of the current `Set` up to `n`
   *           elements.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function take(int $n): Set<Tv>;

  /**
   * Returns a `Set` containing the values of the current `Set` up to but not
   * including the first value that produces `false` when passed to the
   * specified callback.
   *
   * The returned `Set` will always be a proper subset of the current `Set`.
   *
   * @param $fn - The callback that is used to determine the stopping condition.
   *
   * @return - A `Set` that is a proper subset of the current `Set` up until
   *           the callback returns `false`.
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function takeWhile(<<__AtMostRxAsFunc>>(function(Tv): bool) $fn): Set<Tv>;

  /**
   * Returns a `Set` containing the values after the `n`-th element of the
   * current `Set`.
   *
   * The returned `Set` will always be a proper subset of the current `Set`.
   *
   * `n` is 1-based. So the first element is 1, the second 2, etc.
   *
   * @param $n - The last element to be skipped; the `$n+1` element will be
   *             the first one in the returned `Set`.
   *
   * @return - A `Set` that is a proper subset of the current `Set` containing
   *           values after the specified `n`-th element.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function skip(int $n): Set<Tv>;

  /**
   * Returns a `Set` containing the values of the current `Set` starting after
   * and including the first value that produces `true` when passed to the
   * specified callback.
   *
   * The returned `Set` will always be a proper subset of the current `Set`.
   *
   * @param $fn - The callback used to determine the starting element for the
   *              `Set`.
   *
   * @return - A `Set` that is a proper subset of the current `Set` starting
   *           after the callback returns `true`.
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function skipWhile(<<__AtMostRxAsFunc>>(function(Tv): bool) $fn): Set<Tv>;

  /**
   * Returns a subset of the current `Set` starting from a given key up to, but
   * not including, the element at the provided length from the starting key.
   *
   * `$start` is 0-based. `$len` is 1-based. So `slice(0, 2)` would return the
   * elements at key 0 and 1.
   *
   * The returned `Set` will always be a proper subset of the current `Set`.
   *
   * @param $start - The starting value in the current `Set` for the returned
   *                 `Set`.
   * @param $len - The length of the returned `Set`.
   *
   * @return - A `Set` that is a proper subset of the current `Set` starting at
   *           `$start` up to but not including the element `$start + $len`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function slice(int $start, int $len): Set<Tv>;

  /**
   * Returns a `Vector` that is the concatenation of the values of the current
   * `Set` and the values of the provided `Traversable`.
   *
   * The values of the provided `Traversable` is concatenated to the end of the
   * current `Set` to produce the returned `Vector`.
   *
   * @param $traversable - The `Traversable` to concatenate to the current
   *                       `Set`.
   *
   * @return - The concatenated `Vector`.
   *
   * @guide /hack/generics/constraints
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn, __MaybeMutable>>
  public function concat<Tu super Tv>(<<__MaybeMutable, __OnlyRxIfImpl(HH\Rx\Traversable::class)>> Traversable<Tu> $traversable): Vector<Tu>;

  /**
   * Returns the first value in the current `Set`.
   *
   * @return - The first value in the current `Set`, or `null` if the `Set` is
   *           empty.
   */
  <<__Rx, __MaybeMutable>>
  public function firstValue(): ?Tv;

  /**
   * Returns the first "key" in the current `Set`.
   *
   * Since `Set`s do not have keys, it returns the first value.
   *
   * This method is interchangeable with `firstValue()`.
   *
   * @return - The first value in the current `Set`, or `null` if the `Set` is
   *           empty.
   */
  <<__Rx, __MaybeMutable>>
  public function firstKey(): ?arraykey;

  /**
   * Returns the last value in the current `Set`.
   *
   * @return - The last value in the current `Set`, or `null` if the current
   *           `Set` is empty.
   */
  <<__Rx, __MaybeMutable>>
  public function lastValue(): ?Tv;

  /**
   * Returns the last "key" in the current `Set`.
   *
   * Since `Set`s do not have keys, it returns the last value.
   *
   * This method is interchangeable with `lastValue()`.
   *
   * @return - The last value in the current `Set`, or `null` if the current
   *           `Set` is empty.
   */
  <<__Rx, __MaybeMutable>>
  public function lastKey(): ?arraykey;

  /**
   * Checks if the current `Set` is empty.
   *
   * @return - `true` if the current `Set` is empty; `false` otherwise.
   */
  <<__Rx, __MaybeMutable>>
  public function isEmpty(): bool;

  /**
   * Provides the number of elements in the current `Set`.
   *
   * @return - The number of elements in the current `Set`.
   */
  <<__Rx, __MaybeMutable>>
  public function count(): int;

  /**
   * Remove all the elements from the current `Set`.
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __ReturnsVoidToRx>>
  public function clear(): Set<Tv>;

  /**
   * Determines if the specified value is in the current `Set`.
   *
   * @param $v - The value to check.
   * @return - `true` if the specified value is present in the current `Set`;
   *           `false` otherwise.
   */
  <<__Rx, __MaybeMutable>>
  public function contains(mixed $v): bool;

  /**
   * Add the value to the current `Set`.
   *
   * `$set->add($v)` is semantically equivalent to `$set[] = $v` (except that
   * `add()` returns the `Set`).
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $v - The value to add to the current `Set`
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __ReturnsVoidToRx>>
  public function add(Tv $v): Set<Tv>;

  /**
   * For every element in the provided `Traversable`, add the value into the
   * current `Set`.
   *
   * Future changes made to the original `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $k - The `Traversable` with the new values to add. If `null` is
   *             provided, no changes are made.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __AtMostRxAsArgs, __ReturnsVoidToRx>>
  public function addAll(<<__MaybeMutable, __OnlyRxIfImpl(HH\Rx\Traversable::class)>> ?Traversable<Tv> $it): Set<Tv>;

  /**
   * Adds the keys of the specified container to the current `Set` as new
   * values.
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $container - The container with the new keys to add.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __ReturnsVoidToRx>>
  public function addAllKeysOf(
    ?KeyedContainer<Tv,mixed> $container,
  ): Set<Tv>;

  /**
   * Reserves enough memory to accommodate a given number of elements.
   *
   * Reserves enough memory for `sz` elements. If `sz` is less than or equal
   * to the current capacity of this `Set`, this method does nothing.
   *
   * @param $sz - The pre-determined size you want for the current `Set`.
   */
  <<__Rx, __Mutable>>
  public function reserve(int $sz): void;

  /**
   * Removes the specified value from the current `Set`.
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $v - The value to remove.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __ReturnsVoidToRx>>
  public function remove(Tv $v): Set<Tv>;

  /**
   * Removes the values in the current `Set` that are also in the `Traversable`.
   *
   * If a value in the `Traversable` doesn't exist in the current `Set`, that
   * value in the `Traversable` is ignored.
   *
   * Future changes made to the current `Set` ARE reflected in the returned
   * `Set`, and vice-versa.
   *
   * @param $other - The `Traversable` containing values that will be removed
   *                 from the `Set`.
   *
   * @return - Returns itself.
   */
  <<__Rx, __Mutable, __AtMostRxAsArgs, __ReturnsVoidToRx>>
  public function removeAll(<<__MaybeMutable, __OnlyRxIfImpl(HH\Rx\Traversable::class)>> Traversable<Tv> $other): Set<Tv>;

  /**
   * Returns an iterator that points to beginning of the current `Set`.
   *
   * @return - A `KeyedIterator` that allows you to traverse the current `Set`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function getIterator(): HH\Rx\KeyedIterator<arraykey, Tv>;

  /**
   * Returns a `Set` containing the values from the specified `array`.
   *
   * This function is deprecated. Use `new Set ($arr)` instead.
   *
   * @param $arr - The `array` to convert to a `Set`.
   *
   * @return - A `Set` with the values from the provided `array`.
   */
  <<__Deprecated('Use `new Set($arr)` instead.')>>
  public static function fromArray(darray<arraykey, Tv> $arr): Set<Tv>;

  /**
   * Returns a `Set` containing all the values from the specified `array`(s).
   *
   * @param ... - The `array`s to convert to a `Set`.
   *
   * @return - A `Set` with the values from the passed `array`(s).
   */
  <<__Rx, __MutableReturn>>
  public static function fromArrays(...): Set<Tv>;

  /**
   * Creates a `Set` from the given `Traversable`, or an empty `Set` if `null`
   * is passed.
   *
   * This is the static method version of the `Set::__construct()` constructor.
   *
   * @param $items - any `Traversable` object from which to create a `Set`
   *                 (e.g., `array`). If `null`, then an empty `Set` is created.
   *
   * @return - A `Set` with the values from the `Traversable`; or an empty `Set`
   *           if the `Traversable` is `null`.
   */
  <<__Rx, __AtMostRxAsArgs, __MutableReturn>>
  public static function fromItems(<<__MaybeMutable, __OnlyRxIfImpl(HH\Rx\Traversable::class)>> ?Traversable<Tv> $items): Set<Tv>;

  /**
   * Creates a `Set` from the keys of the specified container.
   *
   * The keys of the container will be the values of the `Set`.
   *
   * @param $container - The container with the keys used to create the `Set`.
   *
   * @return - A `Set` built from the keys of the specified container.
   */
  <<__Rx, __MutableReturn>>
  public static function fromKeysOf<Tk as arraykey>(
    ?KeyedContainer<Tk,mixed> $container,
  ): Set<Tk>;

  /**
   * Returns the `string` version of the current `Set`, which is `"Set"`.
   *
   * @return - The `string` `"Set"`.
   */
  <<__Rx, __MaybeMutable>>
  public function __toString(): string;

  /**
   * Returns an `Iterable` view of the current `Set`.
   *
   * The `Iterable` returned is one that produces the values from the current
   * `Set`.
   *
   * @return - The `Iterable` view of the current `Set`.
   */
  <<__Rx, __MutableReturn, __MaybeMutable>>
  public function items(): HH\Rx\Iterable<Tv>;

  <<__Rx, __MaybeMutable>> /* HH_FIXME[0002] */
  public function toVArray(): varray<Tv>;
  <<__Rx, __MaybeMutable>> /* HH_FIXME[0001] */
  public function toDArray(): darray<Tv, Tv>;
}

/**
 * @internal
 *
 * Methods and functions should take and return the KeyedIterator interface.
 */
class SetIterator<+Tv> implements HH\Rx\KeyedIterator<arraykey, Tv> {
  <<__Rx>>
  public function __construct();
  <<__Rx, __MaybeMutable>>
  public function current(): Tv;
  <<__Rx, __MaybeMutable>>
  public function key(): arraykey;
  <<__Rx, __MaybeMutable>>
  public function valid(): bool;
  <<__Rx, __Mutable>>
  public function next(): void;
  <<__Rx, __Mutable>>
  public function rewind(): void;
}
