/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/protocol/types.hpp>

namespace graphene { namespace protocol {

   extern const int64_t scaled_precision_lut[];

   struct price;

   struct asset
   {
      asset( share_type a = 0, asset_id_type id = asset_id_type() )
      :amount(a),asset_id(id){}

      share_type    amount;
      asset_id_type asset_id;

      asset& operator += ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount += o.amount;
         return *this;
      }
      asset& operator -= ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount -= o.amount;
         return *this;
      }
      asset operator -()const { return asset( -amount, asset_id ); }

      friend bool operator == ( const asset& a, const asset& b )
      {
         return std::tie( a.asset_id, a.amount ) == std::tie( b.asset_id, b.amount );
      }
      friend bool operator < ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return a.amount < b.amount;
      }
      friend inline bool operator <= ( const asset& a, const asset& b )
      {
         return !(b < a);
      }

      friend inline bool operator != ( const asset& a, const asset& b )
      {
         return !(a == b);
      }
      friend inline bool operator > ( const asset& a, const asset& b )
      {
         return (b < a);
      }
      friend inline bool operator >= ( const asset& a, const asset& b )
      {
         return !(a < b);
      }

      friend asset operator - ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount - b.amount, a.asset_id );
      }
      friend asset operator + ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount + b.amount, a.asset_id );
      }

      static share_type scaled_precision( uint8_t precision )
      {
         FC_ASSERT( precision < 19 );
         return scaled_precision_lut[ precision ];
      }

      asset multiply_and_round_up( const price& p )const; ///< Multiply and round up
   };

   /**
    * @brief The price struct stores asset prices in the BitShares system.
    *
    * A price is defined as a ratio between two assets, and represents a possible exchange rate between those two
    * assets. prices are generally not stored in any simplified form, i.e. a price of (1000 CORE)/(20 USD) is perfectly
    * normal.
    *
    * The assets within a price are labeled base and quote. Throughout the BitShares code base, the convention used is
    * that the base asset is the asset being sold, and the quote asset is the asset being purchased, where the price is
    * represented as base/quote, so in the example price above the seller is looking to sell CORE asset and get USD in
    * return.
    */
   struct price
   {
      explicit price(const asset& _base = asset(), const asset& _quote = asset())
         : base(_base),quote(_quote){}

      asset base;
      asset quote;

      static price max(asset_id_type base, asset_id_type quote );
      static price min(asset_id_type base, asset_id_type quote );

      static price call_price(const asset& debt, const asset& collateral, uint16_t collateral_ratio);

      /// The unit price for an asset type A is defined to be a price such that for any asset m, m*A=m
      static price unit_price(asset_id_type a = asset_id_type()) { return price(asset(1, a), asset(1, a)); }

      price max()const { return price::max( base.asset_id, quote.asset_id ); }
      price min()const { return price::min( base.asset_id, quote.asset_id ); }

      double to_real()const { return double(base.amount.value)/double(quote.amount.value); }

      bool is_null()const;
      void validate()const;
   };

   price operator / ( const asset& base, const asset& quote );
   inline price operator~( const price& p ) { return price{p.quote,p.base}; }

   bool  operator <  ( const price& a, const price& b );
   bool  operator == ( const price& a, const price& b );

   inline bool  operator >  ( const price& a, const price& b ) { return (b < a); }
   inline bool  operator <= ( const price& a, const price& b ) { return !(b < a); }
   inline bool  operator >= ( const price& a, const price& b ) { return !(a < b); }
   inline bool  operator != ( const price& a, const price& b ) { return !(a == b); }

   asset operator *  ( const asset& a, const price& b ); ///< Multiply and round down

   price operator *  ( const price& p, const ratio_type& r );
   price operator /  ( const price& p, const ratio_type& r );

   inline price& operator *=  ( price& p, const ratio_type& r )
   { return p = p * r; }
   inline price& operator /=  ( price& p, const ratio_type& r )
   { return p = p / r; }

   /**
    *  @class price_feed
    *  @brief defines market parameters for margin positions
    */
   struct price_feed
   {
      /**
       *  Required maintenance collateral is defined
       *  as a fixed point number with a maximum value of 10.000
       *  and a minimum value of 1.000.  (denominated in GRAPHENE_COLLATERAL_RATIO_DENOM)
       *
       *  A black swan event occurs when value_of_collateral equals
       *  value_of_debt, to avoid a black swan a margin call is
       *  executed when value_of_debt * required_maintenance_collateral
       *  equals value_of_collateral using rate.
       *
       *  Default requirement is $1.75 of collateral per $1 of debt
       *
       *  BlackSwan ---> SQR ---> MCR ----> SP
       */
      ///@{
      /**
       * Forced settlements will evaluate using this price, defined as BITASSET / COLLATERAL
       */
      price settlement_price;

      /// Price at which automatically exchanging this asset for CORE from fee pool occurs (used for paying fees)
      price core_exchange_rate;

      /** Fixed point between 1.000 and 10.000, implied fixed point denominator is GRAPHENE_COLLATERAL_RATIO_DENOM */
      uint16_t maintenance_collateral_ratio = GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO;

      /** Fixed point between 1.000 and 10.000, implied fixed point denominator is GRAPHENE_COLLATERAL_RATIO_DENOM */
      uint16_t maximum_short_squeeze_ratio = GRAPHENE_DEFAULT_MAX_SHORT_SQUEEZE_RATIO;

      /** When selling collateral to pay off debt, the least amount of debt to receive should be
       *  min_usd = max_short_squeeze_price() * collateral
       *
       *  This is provided to ensure that a black swan cannot be trigged due to poor liquidity alone, it
       *  must be confirmed by having the max_short_squeeze_price() move below the black swan price.
       */
      price max_short_squeeze_price()const;
      /// Another implementation of max_short_squeeze_price() before the core-1270 hard fork
      price max_short_squeeze_price_before_hf_1270()const;

      /// Call orders with collateralization (aka collateral/debt) not greater than this value are in margin call territory.
      /// Calculation: ~settlement_price * maintenance_collateral_ratio / GRAPHENE_COLLATERAL_RATIO_DENOM
      price maintenance_collateralization()const;

      ///@}

      friend bool operator == ( const price_feed& a, const price_feed& b )
      {
         return std::tie( a.settlement_price, a.maintenance_collateral_ratio, a.maximum_short_squeeze_ratio ) ==
                std::tie( b.settlement_price, b.maintenance_collateral_ratio, b.maximum_short_squeeze_ratio );
      }

      void validate() const;
      bool is_for( asset_id_type asset_id ) const;
   };

   /// @class asset_store
   /// @brief A class to store a quantity of "actual asset" as opposed to a mere amount that does not represent real
   /// asset storage
   ///
   /// The @ref asset_store class provides an error-checking storage for a quantity of asset. This type is intended
   /// to represent a real store of value, as opposed to a documentative note about an amount, which is provided by
   /// the @ref asset type.
   ///
   /// Asset within an asset_store cannot be created or destroyed; it must be moved from store to store. If an
   /// asset_store is destroyed or assigned to (move semantics only) when it still contains asset, an exception is
   /// thrown. Asset can only be added to an asset_store by moving it from another asset_store.
   ///
   /// The exceptions to these rules are for serialization: if an asset_store is destroyed or assigned without having
   /// been modified since it was serialized, no exception is thrown. Also, an asset_store can be created containing
   /// an unchecked amount of asset using the static @ref unchecked_create method.
   class asset_store {
      asset store_amount;
      mutable bool serialized = false;

      void destroy() {
         FC_ASSERT(store_amount.amount == 0 || serialized,
                   "BUG: asset_store destroyed or overwritten with remaining asset inside!");
         serialized = false;
         store_amount.amount = 0;
      }

      /// Helper type for moving asset from one store to another. Either convert to an asset_store, or call @ref to
      /// Intended use is like so:
      /// destination = source.move(100);
      /// source.move(100).to(dest);
      /// This type should be used immediately upon receipt; it cannot be copied or moved.
      class mover {
         asset_store& source;
         share_type amount;
         mover(asset_store& source, share_type amount) : source(source), amount(amount) {}
         mover(const mover&) = default; mover(mover&&) = default;
         friend class asset_store;

      public:
         /// Move the asset to the specified destination
         asset_store& to(asset_store& destination) { return source.to(destination, amount); }
         /// Create a new asset_store and move the asset to it
         operator asset_store() { asset_store result; source.to(result, amount); return result; }
         /// Destroy the asset without causing an exception
         void unchecked_destroy() { asset_store(*this).unchecked_destroy(); }
      };

      struct reflection : public fc::field_reflection<0, asset_store, asset, &asset_store::store_amount> {
         static const asset& get(const asset_store& store) { return store.store_amount; }
      };

   public:
      /// Create an asset_store containing a specified asset, without checking the source of the funds
      static asset_store unchecked_create(asset storage) {
         asset_store store;
         store.store_amount = std::move(storage);
         return store;
      }
      /// Empty an asset_store containing asset without a destination and without throwing an exception
      void unchecked_destroy() {
         store_amount.amount = 0;
      }

      asset_store() = default;
      asset_store(asset_id_type type) { store_amount.asset_id = type; }
      ~asset_store() { destroy(); }
      asset_store(const asset_store&) = delete;
      asset_store(asset_store&& other) {
         store_amount = other.store_amount;
         other.store_amount.amount = 0;
      }
      asset_store& operator=(const asset_store&) = delete;
      asset_store& operator=(asset_store&& other) {
         destroy();
         store_amount = other.store_amount;
         other.store_amount.amount = 0;
         return *this;
      }

      /// Get the asset stored
      const asset& stored_asset() const { return store_amount; }
      /// Get the asset stored
      operator asset() const { return store_amount; }
      /// Get the amount of asset stored
      const share_type& amount() const { return store_amount.amount; }
      /// Get the type of asset stored
      const asset_id_type& asset_type() const { return store_amount.asset_id; }
      /// Check if the store is empty; true if so, false if not
      bool empty() const { return store_amount.amount == 0; }

      /// Move asset from this asset store to another one
      mover move(share_type amount) { return mover{*this, amount}; }
      /// Move the full amount in this asset_store to a destination store; returns a reference to destination
      asset_store& to(asset_store& destination) { return to(destination, amount()); }
      /// Move a specified amount from this asset_store to a destination store; returns a reference to destination
      asset_store& to(asset_store& destination, share_type amount);

      friend bool operator<(const asset_store& a, const asset_store& b) { return a.store_amount < b.store_amount; }
      friend bool operator<=(const asset_store& a, const asset_store& b) { return a.store_amount <= b.store_amount; }
      friend bool operator>(const asset_store& a, const asset_store& b) { return a.store_amount > b.store_amount; }
      friend bool operator>=(const asset_store& a, const asset_store& b) { return a.store_amount >= b.store_amount; }
      friend bool operator==(const asset_store& a, const asset_store& b) { return a.store_amount == b.store_amount; }
      friend bool operator!=(const asset_store& a, const asset_store& b) { return a.store_amount != b.store_amount; }

      void from_variant(const variant& v);
      void to_variant(variant& v) const;
      template<typename DS> void pack(DS& datastream) const;
      template<typename DS> void unpack(DS& datastream);

      struct FC_internal_reflection {
         using members = fc::typelist::list<reflection>;
         template<typename V> static void visit(const V&) {
            constexpr bool No = std::is_integral<V>::value;
            static_assert(No, "asset_store is not compatible with legacy FC reflection visitors");
         }
      };
   };

} } // namespace graphene::protocol

FC_REFLECT( graphene::protocol::asset, (amount)(asset_id) )
FC_REFLECT( graphene::protocol::price, (base)(quote) )

#define GRAPHENE_PRICE_FEED_FIELDS (settlement_price)(maintenance_collateral_ratio)(maximum_short_squeeze_ratio) \
   (core_exchange_rate)

FC_REFLECT( graphene::protocol::price_feed, GRAPHENE_PRICE_FEED_FIELDS )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::asset )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::price )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::price_feed )

inline void graphene::protocol::asset_store::from_variant(const variant& v) {
   serialized = true;
   fc::from_variant(v, store_amount, FC_PACK_MAX_DEPTH);
}
inline void graphene::protocol::asset_store::to_variant(variant& v) const {
   serialized = true;
   fc::to_variant(store_amount, v, FC_PACK_MAX_DEPTH);
}
template<typename DS> void graphene::protocol::asset_store::pack(DS &datastream) const {
   serialized = true;
   fc::raw::pack(datastream, store_amount, FC_PACK_MAX_DEPTH);
}
template<typename DS> void graphene::protocol::asset_store::unpack(DS &datastream) {
   serialized = true;
   fc::raw::unpack(datastream, store_amount, FC_PACK_MAX_DEPTH);
}

namespace fc {
inline void from_variant(const variant& v, graphene::protocol::asset_store& store, uint32_t = 1)
{ store.from_variant(v); }
inline void to_variant(const graphene::protocol::asset_store& store, variant& v, uint32_t = 1)
{ store.to_variant(v); }
namespace raw {
template<typename DS> inline void pack(DS& ds, const graphene::protocol::asset_store& as, uint32_t = 1)
{ as.pack(ds); }
template<typename DS> inline void unpack(DS& ds, graphene::protocol::asset_store& as, uint32_t = 1)
{ as.unpack(ds); }
} } // namespace fc::raw

FC_COMPLETE_INTERNAL_REFLECTION(graphene::protocol::asset_store);