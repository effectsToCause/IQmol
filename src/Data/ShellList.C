/*******************************************************************************
       
  Copyright (C) 2011-2015 Andrew Gilbert
           
  This file is part of IQmol, a free molecular visualization program. See
  <http://iqmol.org> for more details.
       
  IQmol is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  IQmol is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.
      
  You should have received a copy of the GNU General Public License along
  with IQmol.  If not, see <http://www.gnu.org/licenses/>.  
   
********************************************************************************/

#include "ShellList.h"
#include "Geometry.h"
#include "Constants.h"
#include "QsLog.h"
#include <QDebug>
#include <cmath>


namespace IQmol {
namespace Data {

template<> const Type::ID List<Shell>::TypeID = Type::ShellList;


ShellList::ShellList(ShellData const& shellData, Geometry const& geometry) : m_sigBasis(0),
   m_orbitalCoefficients(0)
{
   static double const convExponents(std::pow(Constants::BohrToAngstrom, -2.0));
   unsigned nShells(shellData.shellTypes.size());
   unsigned cnt(0);

   for (unsigned shell = 0; shell < nShells; ++shell) {

       QList<double> expts;
       QList<double> coefs;
       QList<double> coefsSP;

       unsigned atom(shellData.shellToAtom.at(shell)-1);
       qglviewer::Vec pos(geometry.position(atom));

       for (unsigned i = 0; i < shellData.shellPrimitives.at(shell); ++i, ++cnt) {
           // Convert exponents from bohr to angstrom.  The conversion factor
           // for the coefficients depends on the angular momentum and the 
           // conversion is effectively done in the  Shell constructor
           expts.append(shellData.exponents.at(cnt)*convExponents);

           coefs.append(shellData.contractionCoefficients.at(cnt));
           if (!shellData.contractionCoefficientsSP.isEmpty()) {
              coefsSP.append(shellData.contractionCoefficientsSP.at(cnt));
           }   
       }   

       // These cases are from the formatted checkpoint file format
       switch (shellData.shellTypes.at(shell)) {
          case 0:
             append( new Data::Shell(Data::Shell::S, atom, pos, expts, coefs) );
             break;
          case -1: 
             append( new Data::Shell(Data::Shell::S, atom, pos, expts, coefs) );
             append( new Data::Shell(Data::Shell::P, atom, pos, expts, coefsSP) );
             break;
          case 1:
             append( new Data::Shell(Data::Shell::P, atom, pos, expts, coefs) );
             break;
          case -2: 
             append( new Data::Shell(Data::Shell::D5, atom, pos, expts, coefs) );
             break;
          case 2:
             append( new Data::Shell(Data::Shell::D6, atom, pos, expts, coefs) );
             break;
          case -3: 
             append( new Data::Shell(Data::Shell::F7, atom, pos, expts, coefs) );
             break;
          case 3:
             append( new Data::Shell(Data::Shell::F10, atom, pos, expts, coefs) );
             break;
          case -4: 
             append( new Data::Shell(Data::Shell::G9, atom, pos, expts, coefs) );
             break;
          case 4:
             append( new Data::Shell(Data::Shell::G15, atom, pos, expts, coefs) );
             break;

          default:
             QString msg("Unknown Shell type found at position ");
             msg += QString::number(shell);
             msg += ", type: "+ QString::number(shellData.shellTypes.at(shell));
             qDebug() << msg;
             break;
       }   
   }   

   unsigned n(nBasis());
   if (shellData.overlapMatrix.size() == (n+1)*n/2) {
      setOverlapMatrix(shellData.overlapMatrix);
   }   

   resize();
}


ShellList::~ShellList() 
{
   if (m_sigBasis) delete [] m_sigBasis;
}

unsigned ShellList::nBasis() const
{
    unsigned n(0);
    for (int i = 0; i < size(); ++i) {
        n += at(i)->nBasis();
    }
   return n;
}


void ShellList::boundingBox(qglviewer::Vec& min, qglviewer::Vec& max, double const thresh) 
{
   if (isEmpty()) {
      min.x = min.y = min.z = 0.0;
      max.x = max.y = max.z = 0.0;
      return;
   }

   at(0)->boundingBox(min, max, thresh);

   qglviewer::Vec tmin, tmax;
   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       (*shell)->boundingBox(tmin, tmax, thresh);
       min.x = std::min(tmin.x, min.x);
       min.y = std::min(tmin.y, min.y);
       min.z = std::min(tmin.z, min.z);
       max.x = std::max(tmax.x, max.x);
       max.y = std::max(tmax.y, max.y);
       max.z = std::max(tmax.z, max.z);
   }
}


void ShellList::dump() const
{
   unsigned n(0), s(0), p(0), d5(0), d6(0), f7(0), f10(0), g9(0), g15(0);

   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       switch ((*shell)->angularMomentum()) {
          case  Data::Shell::S:    ++s;    n +=  1;  break;
          case  Data::Shell::P:    ++p;    n +=  3;  break;   
          case  Data::Shell::D5:   ++d5;   n +=  5;  break;
          case  Data::Shell::D6:   ++d6;   n +=  6;  break;   
          case  Data::Shell::F7:   ++f7;   n +=  7;  break;
          case  Data::Shell::F10:  ++f10;  n += 10;  break;   
          case  Data::Shell::G9:   ++g9;   n +=  9;  break;
          case  Data::Shell::G15:  ++g15;  n += 15;  break;   
       }   
   }   

   QString check("OK");
   if (n != nBasis()) check.prepend("NOT ");
   qDebug() << "Basis function check:     " << check;

   QString types("   S    P   D5   D6   F7  F10");
   QString tally = QString("%1 %2 %3 %4 %5 %6").arg( s,4).arg( p,4).arg( d5,4)
                                                  .arg(d6,4).arg(f7,4).arg(f10,4);
   qDebug() << "Shell types:              " << types;
   qDebug() << "                          " << tally;
   
   //List<Shell>::dump();
}


void ShellList::resize()
{
   m_nBasis = nBasis();
   m_basisValues.resize(m_nBasis);

   if (m_sigBasis) delete [] m_sigBasis;
   m_sigBasis = new unsigned[m_nBasis];

   unsigned size(m_nBasis*(m_nBasis+1)/2);
   if (2*size != m_nBasis*(m_nBasis+1)) {
      QLOG_WARN() << "Round error in ShellList::resize()";
      ++size;
   }
   m_basisPairValues.resize(size);

   qDebug() << shellAtomOffsets();
   qDebug() << basisAtomOffsets();
}


QList<unsigned> ShellList::shellAtomOffsets() const
{
   QList<unsigned> offsets;
   offsets.append(0);

   unsigned k(0);
   unsigned atomIndex(0);

   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell, ++k) {
       if ((*shell)->atomIndex() != atomIndex) {
          offsets.append(k);
          ++atomIndex;
       }
   }

   return offsets;
}

QList<unsigned> ShellList::basisAtomOffsets() const
{
   QList<unsigned> offsets;
   offsets.append(0);

   unsigned k(0);
   unsigned atomIndex(0);

   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       if ((*shell)->atomIndex() != atomIndex) {
          offsets.append(k);
          ++atomIndex;
       }
       k += (*shell)->nBasis();
   }

   return offsets;
}


// TODO: this could be batched over grid points so that matrix multiplications
// can be used later on.  Also, auxilary data structures could be employed to 
// make the computation more efficient.
Vector const& ShellList::shellValues(qglviewer::Vec const& gridPoint)
{
   double const* values;
   unsigned offset(0);

   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       values = (*shell)->evaluate(gridPoint);
       for (unsigned s = 0; s < (*shell)->nBasis(); ++s, ++offset) {
           m_basisValues[offset] = values[s];
       }
   }

   return m_basisValues;
}


Vector const& ShellList::shellValues(double const x, double const y, double const z)
{
   double const* values;
   unsigned offset(0);

   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       values = (*shell)->evaluate(x, y, z);
       if (values) {
          for (unsigned s = 0; s < (*shell)->nBasis(); ++s, ++offset) {
              m_basisValues[offset] = values[s];
          }
       }else{
          for (unsigned s = 0; s < (*shell)->nBasis(); ++s, ++offset) {
              m_basisValues[offset] = 0;
          }
       }
   }

   return m_basisValues;
}


// DEPRECATE
Vector const& ShellList::shellPairValues(qglviewer::Vec const& gridPoint)
{
   shellValues(gridPoint);

   unsigned k(0);
   double xi, xj; 
   for (unsigned i = 0; i < m_nBasis; ++i) {
       xi = m_basisValues[i];
       for (unsigned j = 0; j < i; ++j, ++k) {
           xj = m_basisValues[j];
           m_basisPairValues[k] = 2.0*xi*xj;
       }   
       m_basisPairValues[k] = xi*xi;
       ++k;
   }

   return m_basisPairValues;
}
// DEPRECATE



void ShellList::setDensityVectors(QList<Vector const*> const& densityVectors)
{
   m_densityVectors = densityVectors;
   m_densityValues.resize(m_densityVectors.size());
}


Vector const& ShellList::densityValues(double const x, double const y, double const z)
{
   unsigned numbas, nSigBas(0), basoff(0);
   double const* values;

   // Determine the significant shells, and corresponding basis function indices
   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       values = (*shell)->evaluate(x,y,z);
       numbas = (*shell)->nBasis();

       if (values) { // only add the significant shells
          for (unsigned i = 0; i < numbas; ++i, ++nSigBas, ++basoff) {
              m_basisValues[nSigBas] = values[i];
              m_sigBasis[nSigBas]    = basoff;
          }
       }else {
          basoff += numbas;
       }
   }

   double   xi, xij; 
   unsigned ii, jj, Ti;
   unsigned nden(m_densityVectors.size());

   for (unsigned k = 0; k < nden; ++k) {
       m_densityValues[k] = 0.0;
   }

   // Now compute the basis function pair values on the grid
   for (unsigned i = 0; i < nSigBas; ++i) {
       xi = m_basisValues[i];
       ii = m_sigBasis[i];
       Ti = (ii*(ii+1))/2;
       for (unsigned j = 0; j < i; ++j) {
           xij = 2.0*xi*m_basisValues[j];
           jj  = m_sigBasis[j];

           for (unsigned k = 0; k < nden; ++k) {
               m_densityValues[k] += 2.0*xij*(*m_densityVectors[k])[Ti+jj];
           }

       }
       
       for (unsigned k = 0; k < nden; ++k) {
           m_densityValues[k] += xi*xi*(*m_densityVectors[k])[Ti+ii];
       }
   }

   return m_densityValues;
}


void ShellList::setOrbitalVectors(Matrix const& coefficients, QList<int> const& indices)
{
   m_orbitalIndices      = indices;
   m_orbitalCoefficients = &coefficients;
   m_orbitalValues.resize(m_orbitalIndices.size());
}


Vector const& ShellList::orbitalValues(double const x, double const y, double const z)
{
   unsigned norb(m_orbitalIndices.size());
   unsigned basoff(0);
   unsigned numbas;
   double const* values;

   for (unsigned k = 0; k < norb; ++k) {
       m_orbitalValues[k] = 0.0;
   }

   // Determine the significant shells, and corresponding basis function indices
   ShellList::const_iterator shell;
   for (shell = begin(); shell != end(); ++shell) {
       values = (*shell)->evaluate(x,y,z);
       numbas = (*shell)->nBasis();

       if (values) { // only add the significant shells
          for (unsigned i = 0; i < numbas; ++i) {
              for (unsigned k = 0; k < norb; ++k) {
                  m_orbitalValues[k] += 
                      (*m_orbitalCoefficients)(m_orbitalIndices[k], basoff+i) * values[i];
              }
          }
       }
       basoff += numbas;
   }

   return m_orbitalValues;
}

} } // end namespace IQmol::Data
