/*
See LICENSE file in root folder
*/
#ifndef ___CU_POOL_MANAGED_OBJECT_H___
#define ___CU_POOL_MANAGED_OBJECT_H___

#include "CastorUtils/Pool/UniqueObjectPool.hpp"

namespace castor
{
	/**
	\author		Sylvain DOREMUS
	\version	0.8.0
	\date		08/01/2016
	\~english
	\brief		Pool managed object.
	\remarks	Uses a policy to change behaviour easily.
	\param		Object		The pool objects type.
	\param		MemDataType	The allocation/deallocation policy type.
	\~french
	\brief		Objet géré par un pool.
	\remarks	Utilisation d'une politique pour permettre de changer le comportement assez facilement.
	\param		Object		Le type d'objet.
	\param		MemDataType	Le type de la politique d'allocation/désallocation.
	*/
	template< typename Object, MemoryDataType MemDataType >
	class PoolManagedObject
		: public Object
	{
	public:
		using MyUniqueObjectPool = UniqueObjectPool< Object, MemDataType >;

	public:
		/**
		 *\~english
		 *\brief		Constructor.
		 *\param[in]	params	The Object constructor parameters.
		 *\~french
		 *\brief		Constructeur.
		 *\param[in]	params	Les paramètre du constructeur de Object.
		 */
		template< typename ... Params >
		explicit PoolManagedObject( Params ... params )noexcept
			: Object( params... )
		{
		}
		/**
		 *\~english
		 *\brief		Copy constructor.
		 *\param[in]	rhs	The other object.
		 *\~french
		 *\brief		Constructeur par copie.
		 *\param[in]	rhs	L'autre objet.
		 */
		PoolManagedObject( PoolManagedObject const & rhs )noexcept
			: Object( rhs )
		{
		}
		/**
		 *\~english
		 *\brief		Copy constructor.
		 *\param[in]	rhs	The other object.
		 *\~french
		 *\brief		Constructeur par copie.
		 *\param[in]	rhs	L'autre objet.
		 */
		explicit PoolManagedObject( Object const & rhs )noexcept
			: Object( rhs )
		{
		}
		/**
		 *\~english
		 *\brief		New operator overload.
		 *\remarks		Uses the objects pool to allocate memory.
		 *\param[in]	size	The allocation size.
		 *\~french
		 *\brief		Surcharge de l'opérateur new.
		 *\remarks		Utilise le pool d'objets pour allouer la mémoire.
		 *\param[in]	size	La taille à allouer.
		 */
		static void * operator new( std::size_t size )
		{
			auto result = MyUniqueObjectPool::get().allocate();

			if ( !result )
			{
				throw std::bad_alloc{};
			}

			return result;
		}
		/**
		 *\~english
		 *\brief		Delete operator overload.
		 *\remarks		Uses the objects pool to deallocate memory.
		 *\param[in]	ptr	The pointer to deallocate.
		 *\param[in]	size	The deallocation size.
		 *\~french
		 *\brief		Surcharge de l'opérateur delete.
		 *\remarks		Utilise le pool d'objets pour désallouer la mémoire.
		 *\param[in]	ptr	Le pointeur à désallouer.
		 *\param[in]	size	La taille à désallouer.
		 */
		static void operator delete( void * ptr, std::size_t size )
		{
			MyUniqueObjectPool::get().deallocate( reinterpret_cast< Object * >( ptr ) );
		}
		/**
		 *\~english
		 *\brief		Delete array operator overload.
		 *\remarks		Uses the objects pool to deallocate memory.
		 *\param[in]	ptr	The pointer to deallocate.
		 *\param[in]	size	The deallocation size.
		 *\~french
		 *\brief		Surcharge de l'opérateur delete de tableau.
		 *\remarks		Utilise le pool d'objets pour désallouer la mémoire.
		 *\param[in]	ptr	Le pointeur à désallouer.
		 *\param[in]	size	La taille à désallouer.
		 */
		static void operator delete[]( void * ptr, std::size_t size )
		{
			MyUniqueObjectPool::get().deallocate( reinterpret_cast< Object * >( ptr ) );
		}
	};
}

#endif
