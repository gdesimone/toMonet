#include "cts/infra/QueryGraph.hpp"
#include "cts/semana/SemanticAnalysis.hpp"
#include "cts/parser/SPARQLLexer.hpp"
#include "cts/parser/SPARQLParser.hpp"
#include "rts/database/Database.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <map>

//---------------------------------------------------------------------------
// RDF-3X
// (c) 2008 Thomas Neumann. Web site: http://www.mpi-inf.mpg.de/~neumann/rdf3x
//
// This work is licensed under the Creative Commons
// Attribution-Noncommercial-Share Alike 3.0 Unported License. To view a copy
// of this license, visit http://creativecommons.org/licenses/by-nc-sa/3.0/
// or send a letter to Creative Commons, 171 Second Street, Suite 300,
// San Francisco, California, 94105, USA.
//---------------------------------------------------------------------------
using namespace std;

//---------------------------------------------------------------------------
static string readInput(istream& in)
// Read the input query
{
  string result;
  while (true) {
    string s;
    getline(in,s);
    if (!in.good())
      break;
    result+=s;
    result+='\n';
  }
  return result;
}
//---------------------------------------------------------------------------
static string buildFactsAttribute(unsigned id,const char* attribute)
// Build the attribute name for a facts attribute
{
  char buffer[50];
  snprintf(buffer,sizeof(buffer),"f%u.%s",id,attribute);
  return string(buffer);
}

static void translateSubQueryMonet(QueryGraph& query, QueryGraph::SubQuery subquery, map<unsigned,unsigned>& projection, set<unsigned> unionvars, map<unsigned,unsigned>& null) {

  // dictionary with the nodes elements
  map <unsigned, string> representative;

  // esto creo que va en el translateMonet
  {
    unsigned id=0;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if ((!(*iter).constSubject)&&(!representative.count((*iter).subject)))
	representative[(*iter).subject]=buildFactsAttribute(id,"subject");
      if ((!(*iter).constPredicate)&&(!representative.count((*iter).predicate)))
	representative[(*iter).predicate]=buildFactsAttribute(id,"predicate");
      if ((!(*iter).constObject)&&(!representative.count((*iter).object)))
	representative[(*iter).object]=buildFactsAttribute(id,"object");
      ++id;
    }
  }
  cout << "   select ";
  {
    unsigned id=0, i=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      
      // este if deberia tener los if que se encarguen de los nulls para el caso del union
      if (i) cout << ",";
      // si no esta 
      if(!projection.count(*iter)) {
	if(representative.count(*iter)) {
          //cout << " aca " << endl;
	  cout << representative[*iter];
	  projection[*iter]=1;
	  null[*iter] = 1;
	}
	else if (!representative.count(*iter) && null.count(*iter))
	  cout << " NULL";
	cout << " as r" << id;
	i=1;
      
      } else {
	if (representative.count(*iter)) {
	  cout << representative[*iter] << " as r" << id;
	  i=1;
	} else if (null.count(*iter)) {
	  cout << " NULL as r " << id;
	  i = 1;
	}
      }
	    
      id++;
    }
  }

  cout << endl;
  cout << "   from ";
  {
    unsigned id=0;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if (id) cout << ",";
      if ((*iter).constPredicate)
	cout << "p" << (*iter).predicate << " f" << id; else
	cout << "allproperties f" << id;
      ++id;
    }

  }
  cout << endl;
  cout << "   where ";
  {
    unsigned id=0; bool first=true;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      string s=buildFactsAttribute(id,"subject"),p=buildFactsAttribute(id,"predicate"),o=buildFactsAttribute(id,"object");
      if ((*iter).constSubject) {
	if (first) first=false; else cout << " and ";
	cout << s << "=" << (*iter).subject;
      } else if (representative[(*iter).subject]!=s) {
	if (first) first=false; else cout << " and ";
	cout << s << "=" << representative[(*iter).subject];
      }
      if ((*iter).constPredicate) {
      } else if (representative[(*iter).predicate]!=p) {
	if (first) first=false; else cout << " and ";
	cout << p << "=" << representative[(*iter).predicate];
      }
      if ((*iter).constObject) {
	if (first) first=false; else cout << " and ";
	cout << o << "=" << (*iter).object;
      } else if (representative[(*iter).object]!=o) {
	if (first) first=false; else cout << " and ";
	cout << o << "=" << representative[(*iter).object];
      }
      ++id;
    }
  }
} 

static void translateUnionMonet(QueryGraph& query, vector<QueryGraph::SubQuery>  unions, map<unsigned,unsigned>& projection, set<unsigned> unionvars) {

  unsigned i = 0;
  map<unsigned,unsigned> null;
  for (vector<QueryGraph::SubQuery>::const_iterator iter=unions.begin(), limit=unions.end() ; iter != limit ; ++iter) {
   if (i) 
     cout << "\n   UNION" << endl;
   translateSubQueryMonet(query,*iter,projection,unionvars,null);
   i++;
  }
}

static void translateOptionalMonet(QueryGraph& query, QueryGraph::SubQuery subquery,map<unsigned,unsigned>& projection, unsigned& f, unsigned& r, unsigned& fact, unsigned factbool) {

  unsigned fact_ini= fact;
  if (factbool && subquery.nodes.size()) {
    //    translateSubQueryOptional(query, subquery, f,r, projection, set<unsigned>());
    cout << "facts" << fact_ini;
  }
  
  cout << " Fin Translate Optional Monet ";
  // deberia revisar si es la primera vez y si nodes tiene algo
  // luego ir al translateSubQuery del Optional
  
  // 
  


}  


static void translateMonet(QueryGraph& query, QueryGraph::SubQuery subquery){

  map <unsigned, unsigned> projection, null;
  
  // No optional, No Union
  if(!subquery.optional.size() && !subquery.unions.size()) {
    translateSubQueryMonet(query, subquery, projection, set<unsigned>(), null);
  }

  // Union 
  else if(!subquery.optional.size() && subquery.unions.size() == 1) {
    translateUnionMonet(query, subquery.unions[0],projection, set<unsigned>());
  }

  // Optional clause
  else if(subquery.optional.size() == 1 && !subquery.unions.size()) {
    cout << " hola ";
    unsigned  f = 0, r = 0, fact = 0;
    translateOptionalMonet(query,subquery, projection, f,r,fact,1);
  }
}



//---------------------------------------------------------------------------
static void dumpMonetDB(QueryGraph& query)
// Dump a monet query
{
  cout << "select ";
  {
    unsigned id=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      if (id) cout << ",";
      cout << "s" << id << ".value";
      id++;
    }
  }
  cout << endl;
  cout << "from (" << endl;
 
  // Este map no aparece en el dumpPostgres 
  // map<unsigned,string> representative;

  QueryGraph::SubQuery subquery = query.getQuery();
  
  translateMonet(query,subquery);

  
  // // esto creo que va en el translateMonet
  // {
  //   unsigned id=0;
  //   for (vector<QueryGraph::Node>::const_iterator iter=query.getQuery().nodes.begin(),limit=query.getQuery().nodes.end();iter!=limit;++iter) {
  //     if ((!(*iter).constSubject)&&(!representative.count((*iter).subject)))
  // 	representative[(*iter).subject]=buildFactsAttribute(id,"subject");
  //     if ((!(*iter).constPredicate)&&(!representative.count((*iter).predicate)))
  // 	representative[(*iter).predicate]=buildFactsAttribute(id,"predicate");
  //     if ((!(*iter).constObject)&&(!representative.count((*iter).object)))
  // 	representative[(*iter).object]=buildFactsAttribute(id,"object");
  //     ++id;
  //   }
  // }
  // cout << "   select ";
  // {
  //   unsigned id=0;
  //   for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
  //     if (id) cout << ",";
  //     cout << representative[*iter] << " as r" << id;
  //     id++;
  //   }
  // }
  // cout << endl;
  // cout << "   from ";
  // {
  //   unsigned id=0;
  //   for (vector<QueryGraph::Node>::const_iterator iter=query.getQuery().nodes.begin(),limit=query.getQuery().nodes.end();iter!=limit;++iter) {
  //     if (id) cout << ",";
  //     if ((*iter).constPredicate)
  // 	cout << "p" << (*iter).predicate << " f" << id; else
  // 	cout << "allproperties f" << id;
  //     ++id;
  //   }

  // }
  // cout << endl;
  // cout << "   where ";
  // {
  //   unsigned id=0; bool first=true;
  //   for (vector<QueryGraph::Node>::const_iterator iter=query.getQuery().nodes.begin(),limit=query.getQuery().nodes.end();iter!=limit;++iter) {
  //     string s=buildFactsAttribute(id,"subject"),p=buildFactsAttribute(id,"predicate"),o=buildFactsAttribute(id,"object");
  //     if ((*iter).constSubject) {
  // 	if (first) first=false; else cout << " and ";
  // 	cout << s << "=" << (*iter).subject;
  //     } else if (representative[(*iter).subject]!=s) {
  // 	if (first) first=false; else cout << " and ";
  // 	cout << s << "=" << representative[(*iter).subject];
  //     }
  //     if ((*iter).constPredicate) {
  //     } else if (representative[(*iter).predicate]!=p) {
  // 	if (first) first=false; else cout << " and ";
  // 	cout << p << "=" << representative[(*iter).predicate];
  //     }
  //     if ((*iter).constObject) {
  // 	if (first) first=false; else cout << " and ";
  // 	cout << o << "=" << (*iter).object;
  //     } else if (representative[(*iter).object]!=o) {
  // 	if (first) first=false; else cout << " and ";
  // 	cout << o << "=" << representative[(*iter).object];
  //     }
  //     ++id;
  //   }
  // }


  // desde aquí llega el dumpPotgres 
  cout << endl << ") facts";
  {
    unsigned id=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      cout << ",strings s" << id;
      id++;
    }
  }
  cout << endl;
  cout << "where ";
  {
    unsigned id=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      if (id) cout << " and ";
      cout << "s" << id << ".id=facts.r" << id;
      id++;
    }
  }
  cout << ";" << endl;
}
//---------------------------------------------------------------------------
static string databaseName(const char* fileName)
// Guess the database name from the file name
{
  const char* start=fileName;
  for (const char* iter=fileName;*iter;++iter)
    if ((*iter)=='/')
      start=iter+1;
  const char* stop=start;
  while ((*stop)&&((*stop)!='.'))
    ++stop;
  return string(start,stop);
}
//---------------------------------------------------------------------------
static void translateFilter(QueryGraph::Filter filter, map<unsigned,unsigned> var) {
  map<QueryGraph::Filter::Type,string> binaryOperator; 
  binaryOperator[QueryGraph::Filter::Or]             = "or";
  binaryOperator[QueryGraph::Filter::And]            = "and";
  binaryOperator[QueryGraph::Filter::Equal]          = "=";
  binaryOperator[QueryGraph::Filter::NotEqual]       = "<>";
  binaryOperator[QueryGraph::Filter::Less]           = "<";
  binaryOperator[QueryGraph::Filter::LessOrEqual]    = "<=";
  binaryOperator[QueryGraph::Filter::Greater]        = ">";
  binaryOperator[QueryGraph::Filter::GreaterOrEqual] = ">=";
  binaryOperator[QueryGraph::Filter::Plus]           = "+";
  binaryOperator[QueryGraph::Filter::Minus]          = "-";
  binaryOperator[QueryGraph::Filter::Mul]            = "*";
  binaryOperator[QueryGraph::Filter::Div]            = "/";

  map<QueryGraph::Filter::Type,string> unaryOperator;
  unaryOperator[QueryGraph::Filter::Not]        = "not";
  unaryOperator[QueryGraph::Filter::UnaryPlus]  = "+";
  unaryOperator[QueryGraph::Filter::UnaryMinus] = "-"; 
  
  if (filter.type == QueryGraph::Filter::Variable) 
    cout << "trim(both \'\"\' from m" << var[filter.id] << ".value)::int"; //We forced cast to INT type
  else if (filter.type == QueryGraph::Filter::Literal)
    cout << filter.value;
  else if (filter.type == QueryGraph::Filter::IRI)
    cout << "\'" << filter.value << "\'";
  else if (unaryOperator.count(filter.type)) {
    cout << unaryOperator[filter.type] << "(";
    translateFilter(*filter.arg1,var);
    cout << ")";
  }
  else if (binaryOperator.count(filter.type)){
      cout << "(";
      translateFilter(*filter.arg1,var);
      cout << " " << binaryOperator[filter.type] << " ";
      translateFilter(*filter.arg2,var);
      cout << ")";
  }
}

/*static void translateInsideFilter(QueryGraph::Filter& filter, map<unsigned,string> representative) {
  if (filter.type == QueryGraph::Filter::Equal || filter.type == QueryGraph::Filter::NotEqual) {
    cout << " and ";
    if (filter.arg1->type ==  QueryGraph::Filter::Variable && filter.arg2->type ==  QueryGraph::Filter::IRI) {
      cout << representative[filter.arg1->id]; 
      if (filter.type == QueryGraph::Filter::Equal)
        cout << "=";
      else if (filter.type == QueryGraph::Filter::NotEqual)
        cout << "<>";
      cout << filter.arg2->value;
    }
    else if (filter.arg2->type ==  QueryGraph::Filter::Variable && filter.arg1->type ==  QueryGraph::Filter::IRI) {
      cout << representative[filter.arg2->id];
      if (filter.type == QueryGraph::Filter::Equal)
        cout << "=";
      else if (filter.type == QueryGraph::Filter::NotEqual)
        cout << "<>";
      cout << filter.arg1->id;
    }
  }
}

static void insertInSet(set<unsigned> set1, set<unsigned>& set2) {
  for(set<unsigned>::iterator iter=set1.begin(),limit=set1.end();iter!=limit;++iter)
    set2.insert(*iter);
}*/

//---------------------------------------------------------------------------

static void set2map(set<unsigned> var, map<unsigned,unsigned>& var2pos) {
  unsigned i=0;

  for (set<unsigned>::const_iterator iter=var.begin(),limit=var.end();iter!=limit;++iter) {
    var2pos[*iter] = i;
    i++;
  }
}

static void getVariablesF(QueryGraph::Filter filter, set<unsigned>& vars) {
  if (filter.type == QueryGraph::Filter::Variable)
    vars.insert(filter.id);
  else if ((filter.type == QueryGraph::Filter::Or) || (filter.type ==  QueryGraph::Filter::And) || 
           (filter.type == QueryGraph::Filter::Equal) || (filter.type == QueryGraph::Filter::NotEqual) || 
           (filter.type == QueryGraph::Filter::Less) || (filter.type == QueryGraph::Filter::LessOrEqual)|| 
           (filter.type == QueryGraph::Filter::Greater) || (filter.type == QueryGraph::Filter::GreaterOrEqual) || 
           (filter.type == QueryGraph::Filter::Plus) || (filter.type == QueryGraph::Filter::Minus) || 
           (filter.type == QueryGraph::Filter::Mul) || (filter.type == QueryGraph::Filter::Div)) {
    getVariablesF(*filter.arg1,vars);
    getVariablesF(*filter.arg2,vars);
  }
  else if ((filter.type == QueryGraph::Filter::Not) || 
           (filter.type == QueryGraph::Filter::UnaryPlus) || 
           (filter.type == QueryGraph::Filter::UnaryMinus))
    getVariablesF(*filter.arg1,vars);

}

static void getVariablesVF(vector<QueryGraph::Filter> filters, set<unsigned>& vars) {
  for (vector<QueryGraph::Filter>::const_iterator iter=filters.begin(),limit=filters.end();iter!=limit;++iter) {
/*    if ((*iter).type == QueryGraph::Filter::Equal || (*iter).type == QueryGraph::Filter::NotEqual){
      if (!((*iter).arg1->type ==  QueryGraph::Filter::Variable && (*iter).arg2->type ==  QueryGraph::Filter::IRI) &&
          !((*iter).arg2->type ==  QueryGraph::Filter::Variable && (*iter).arg1->type ==  QueryGraph::Filter::IRI)) {
        getVariablesF(*iter,vars);
      }
    }
    else */
      getVariablesF(*iter,vars);
  }
}

//---------------------------------------------------------------------------
static void translateSubQuery(QueryGraph& query, QueryGraph::SubQuery subquery, map<unsigned,unsigned>& projection, const string& schema, set<unsigned> unionvars, map<unsigned,unsigned>& null) {
  //Create dictionary with elements of nodes.
  map<unsigned,string> representative;
  {
    unsigned id=0;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if ((!(*iter).constSubject)&&(!representative.count((*iter).subject)))
	representative[(*iter).subject]=buildFactsAttribute(id,"subject");
      if ((!(*iter).constPredicate)&&(!representative.count((*iter).predicate)))
	representative[(*iter).predicate]=buildFactsAttribute(id,"predicate");
      if ((!(*iter).constObject)&&(!representative.count((*iter).object)))
	representative[(*iter).object]=buildFactsAttribute(id,"object");
      ++id;
    }
  }
  //Translate SELECT clause but values are id's. 
  cout << "   (select ";
  {
    unsigned id=0, i=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) { 
      if (i) cout << ",";
      //Translate projection variables without repeat it.
      if (!projection.count(*iter)) {
        if (representative.count(*iter)) {
	  cout << representative[*iter];
          projection[*iter]=1;
          null[*iter]=1;
        }
        //Translate to NULL when UNION is translated.
        else if (!representative.count(*iter) && null.count(*iter))
          cout << "NULL";       
        cout << " as r" << id;
        i =1;
      }
      else {
        if (representative.count(*iter)) {
          cout << representative[*iter] << " as r" << id;
          i=1;
        }
        else if (null.count(*iter)) {
           cout << "NULL as r" << id;
           i =1;
        }
      }
      id++;
    }
    id=0;
    //Projection variables for JOIN when OPTIONAL is translated.
    for(set<unsigned>::iterator iter=unionvars.begin(),limit=unionvars.end();iter!=limit;++iter){
      if (i) cout << ","; 
      if (representative.count(*iter)) 
        cout << representative[*iter]; 
      else 
        cout << "NULL"; 
      cout << " as q" << id;
      i=1;
      id++;
    }
  }
  //One relation fx for node in the Group Graph Pattern
  cout << endl;
  cout << "    from ";
  {
    unsigned id=0;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if (id) cout << ",";
      cout << schema << ".facts f" << id;
      ++id;
    }

  }
  //Translate filters
  set<unsigned> varsfilters;
  if (subquery.filters.size()) {
    cout << ",";
    getVariablesVF(subquery.filters,varsfilters);
    unsigned id=0;
    //Translate for get value of id
    for(set<unsigned>::const_iterator iter=varsfilters.begin(),limit=varsfilters.end();iter!=limit;iter++) {
      if(id) cout << ",";
      cout << schema << ".strings m" << id;
      id++;
    }
  }
  cout << endl;
  cout << "    where ";
  //Join conditions between nodes.
  {
    unsigned id=0; bool first=true;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      string s=buildFactsAttribute(id,"subject"),p=buildFactsAttribute(id,"predicate"),o=buildFactsAttribute(id,"object");
      if ((*iter).constSubject) {
	if (first) first=false; else cout << " and ";
	cout << s << "=" << (*iter).subject;
      } else if (representative[(*iter).subject]!=s) {
	if (first) first=false; else cout << " and ";
	cout << s << "=" << representative[(*iter).subject];
      }
      if ((*iter).constPredicate) {
	if (first) first=false; else cout << " and ";
	cout << p << "=" << (*iter).predicate;
      } else if (representative[(*iter).predicate]!=p) {
	if (first) first=false; else cout << " and ";
	cout << p << "=" << representative[(*iter).predicate];
      }
      if ((*iter).constObject) {
	if (first) first=false; else cout << " and ";
	cout << o << "=" << (*iter).object;
      } else if (representative[(*iter).object]!=o) {
	if (first) first=false; else cout << " and ";
	cout << o << "=" << representative[(*iter).object];
      }
      ++id;
    }
  }
  //Join for get values for filters.
  if (subquery.filters.size()) {
    map<unsigned,unsigned> varsfilters_map;
    set2map(varsfilters,varsfilters_map);
    unsigned id = 0;
    for(set<unsigned>::const_iterator iter=varsfilters.begin(),limit=varsfilters.end();iter!=limit;iter++){
      cout << " and m" << id << ".id=" << representative[*iter];
      id++;
    }
    //Finally, translate filters.
    for(vector<QueryGraph::Filter>::iterator iter = subquery.filters.begin(), limit = subquery.filters.end() ; iter != limit ; iter++) {
      cout << " and "; 
      translateFilter(*iter,varsfilters_map);
    }
  }
  cout << ")";
}

static void getVariables(QueryGraph::SubQuery subquery, set<unsigned>& vars) {
   //Get variables of Basic Group Graph Pattern
   for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if ((!(*iter).constSubject))
        vars.insert((*iter).subject);
      if ((!(*iter).constPredicate))
        vars.insert((*iter).predicate);
      if ((!(*iter).constObject))
        vars.insert((*iter).object);
    }
}

static void getVariablesVector(vector<QueryGraph::SubQuery> subqueries, set<unsigned>& vars) {
  //Get variables of a vector of Basic Group Graph Pattern
  for (vector<QueryGraph::SubQuery>::iterator iter1=subqueries.begin() , limit1=subqueries.end() ; iter1!=limit1 ; ++iter1)
    getVariables(*iter1,vars);
}

static void intersect(set<unsigned> set1, set<unsigned> set2, set<unsigned>& intersect) {
    //Intersect two sets and storage intersection in another set variable
    for(set<unsigned>::iterator iter1=set1.begin(),limit1=set1.end();iter1!=limit1;++iter1) 
      for(set<unsigned>::iterator iter2=set2.begin(),limit2=set2.end();iter2!=limit2;++iter2)
        if ((*iter1) == (*iter2))
          intersect.insert(*iter1);
} 

//---------------------------------------------------------------------------
static void translateSubQueryOptional(QueryGraph& query, QueryGraph::SubQuery subquery, const string& schema,unsigned& f, unsigned& r, map<unsigned,unsigned>& projection, set<unsigned> optionalvars) {
  map<unsigned,string> representative;
  set<unsigned> vars1; 
  vector<set<unsigned> > vars2;
  set<unsigned> common;

  {
    unsigned id=f;
    //Create dictionary with elements of nodes.
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if ((!(*iter).constSubject)&&(!representative.count((*iter).subject)))
	representative[(*iter).subject]=buildFactsAttribute(id,"subject");
      if ((!(*iter).constPredicate)&&(!representative.count((*iter).predicate)))
	representative[(*iter).predicate]=buildFactsAttribute(id,"predicate");
      if ((!(*iter).constObject)&&(!representative.count((*iter).object)))
	representative[(*iter).object]=buildFactsAttribute(id,"object");
      ++id;
    }
  }
  getVariables(subquery,vars1);
  //OPTIONAL inside this OPTIONAL clause? Get variables for JOIN
  if (subquery.optional.size()) {
    unsigned i=0;
    vars2.resize(subquery.optional.size());
    common.clear();
    for(vector<QueryGraph::SubQuery>::iterator iter=subquery.optional.begin(),limit=subquery.optional.end();iter!=limit;++iter) {
      getVariables(*iter,vars2[i]);
      intersect(vars1,vars2[i],common);
      intersect(vars2[i],vars1,common);
      i++;
      if ((*iter).unions.size() == 1) {
        getVariablesVector((*iter).unions[0],vars2[0]);
        intersect(vars1,vars2[0],common);
        intersect(vars2[0],vars1,common);
      }
    }
  }
  
  cout << "   (select ";
  {
    unsigned id=r, tmp=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      //Don't repeat variables in SELECT clause
      if (representative.count(*iter) && !projection.count(*iter)) {
        projection[*iter] = 1;
        if (tmp) cout << ",";
        cout << representative[*iter] << " as r" << id;
        id++;
        tmp=1;
      }
    }
    r = id;
  }
  {
  unsigned id = 0;
  //Select variables from JOIN of OPTIONAL
  if (optionalvars.size())
    for(set<unsigned>::iterator iter=optionalvars.begin(),limit=optionalvars.end();iter!=limit;++iter){ 
      cout << "," << representative[*iter] << " as q" << id;
      id++;
    }

  id = 0;
  //Select variables to JOIN of OPTIONAL
  if (subquery.optional.size() || subquery.unions[0].size() == 1)
    for(set<unsigned>::iterator iter=common.begin(),limit=common.end();iter!=limit;++iter) {
      cout << "," << representative[*iter] << " as p" << id;
      id++;
    }
  }

  cout << endl;
  cout << "    from ";
  {
    unsigned id=f;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      if (iter != subquery.nodes.begin()) cout << ",";
      cout << schema << ".facts f" << id;
      ++id;
    }   
  }

  set<unsigned> varsfilters;
  if (subquery.filters.size()) {
    cout << ",";
    getVariablesVF(subquery.filters,varsfilters);
    unsigned id=0;
    for(set<unsigned>::const_iterator iter=varsfilters.begin(),limit=varsfilters.end();iter!=limit;iter++) {
      if(id) cout << ",";
      cout << schema << ".strings m" << id;
      id++;
    }
  }
 cout << endl;
  cout << "    where ";
  {
    unsigned id=f; bool first=true;
    for (vector<QueryGraph::Node>::const_iterator iter=subquery.nodes.begin(),limit=subquery.nodes.end();iter!=limit;++iter) {
      string s=buildFactsAttribute(id,"subject"),p=buildFactsAttribute(id,"predicate"),o=buildFactsAttribute(id,"object");
      if ((*iter).constSubject) {
	if (first) first=false; else cout << " and ";
	cout << s << "=" << (*iter).subject;
      } else if (representative[(*iter).subject]!=s) {
	if (first) first=false; else cout << " and ";
	cout << s << "=" << representative[(*iter).subject];
      }
      if ((*iter).constPredicate) {
	if (first) first=false; else cout << " and ";
	cout << p << "=" << (*iter).predicate;
      } else if (representative[(*iter).predicate]!=p) {
	if (first) first=false; else cout << " and ";
	cout << p << "=" << representative[(*iter).predicate];
      }
      if ((*iter).constObject) {
	if (first) first=false; else cout << " and ";
	cout << o << "=" << (*iter).object;
      } else if (representative[(*iter).object]!=o) {
	if (first) first=false; else cout << " and ";
	cout << o << "=" << representative[(*iter).object];
      }
      ++id;
    }
    f=id;
  }

 if (subquery.filters.size()) {
    map<unsigned,unsigned> varsfilters_map;
    set2map(varsfilters,varsfilters_map);
    unsigned id = 0;
    for(set<unsigned>::const_iterator iter=varsfilters.begin(),limit=varsfilters.end();iter!=limit;iter++) {
      cout << " and m" << id << ".id=" << representative[*iter];
      id++;
    }

    for(vector<QueryGraph::Filter>::iterator iter = subquery.filters.begin(), limit = subquery.filters.end() ; iter != limit ; iter++) {
      cout << " and ";
      translateFilter(*iter,varsfilters_map);
    }
  }
 
  cout << ")";
}

static void onOptional(set<unsigned> commons, unsigned fact_ini, unsigned fact) {       
  cout << "facts" << fact << "\n    ON (";
  unsigned j = 0;
  for (set<unsigned>::iterator iter1=commons.begin(),limit1=commons.end();iter1!=limit1;++iter1){
    if (j) cout << " and "; 
    cout << "facts" << fact_ini << ".p" << j << " = facts" << fact << ".q" << j;
    j++;
  }
  cout << ")";
}
 //---------------------------------------------------------------------------
static void translateUnion(QueryGraph& query, vector<QueryGraph::SubQuery> unions, map<unsigned,unsigned>& projection, const string& schema, set<unsigned> unionvars) {
  unsigned id = 0;
  map<unsigned,unsigned> null;
  for (vector<QueryGraph::SubQuery>::const_iterator iter=unions.begin(), limit=unions.end() ; iter != limit ; ++iter) {
   if (id) 
     cout << "\n   UNION" << endl;
   translateSubQuery(query,*iter,projection,schema,unionvars,null);
   id++;
  }
}


//---------------------------------------------------------------------------
static void translateOptional(QueryGraph& query, QueryGraph::SubQuery subquery, map<unsigned,unsigned>& projection, const string& schema, unsigned& f, unsigned& r, unsigned& fact, unsigned factbool) {
  unsigned fact_ini = fact;
  if (factbool && subquery.nodes.size()) {
    translateSubQueryOptional(query,subquery,schema,f,r,projection,set<unsigned>());
    cout << "facts" << fact_ini;
  }
  
  set<unsigned> vars1;  
  vector<set<unsigned> > commons, vars;
  unsigned i=0;

  getVariables(subquery,vars1);
  vars.resize(subquery.optional.size());
  commons.resize(subquery.optional.size());
  for(vector<QueryGraph::SubQuery>::iterator iter=subquery.optional.begin(),limit=subquery.optional.end();iter!=limit;++iter) { 
    cout << "\n    LEFT OUTER JOIN" << endl;
    fact++;
    //Translate OPTIONAL clause inside OPTIONAL clause
    if ((*iter).nodes.size()) {
      getVariables(*iter,vars[i]);
      intersect(vars1,vars[i],commons[i]);
      intersect(vars[i],vars1,commons[i]);
      translateSubQueryOptional(query,*iter,schema,f,r,projection,commons[i]);
      onOptional(commons[i],fact_ini,fact);
      if ((*iter).optional.size())
        translateOptional(query,*iter,projection,schema,f,r,fact,0);
    }
    else {
      //Translate UNION clause inside OPTIONAL clause 
      if ((*iter).unions.size() == 1){
        set<unsigned> vars2;
        getVariablesVector((*iter).unions[0],vars2);
        intersect(vars1,vars2,commons[i]);
        intersect(vars2,vars1,commons[i]);
	cout << "(";
        translateUnion(query,(*iter).unions[0],projection,schema,commons[i]);
	cout << ")";
        onOptional(commons[i],fact_ini,fact);
      }
    }
    i++;
  }
}
//---------------------------------------------------------------------------
static void translatePostgres(QueryGraph& query, QueryGraph::SubQuery subquery, const string& schema) {
  map<unsigned,unsigned> projection,null;
  // No OPTIONAL, No UNION
  if (!subquery.optional.size() && !subquery.unions.size()) {
    translateSubQuery(query,subquery,projection,schema,set<unsigned>(),null);
  }
  // Only UNION clause
  else if (!subquery.optional.size() && subquery.unions.size() == 1) {
    translateUnion(query,subquery.unions[0],projection,schema,set<unsigned>());
  }
  // Only OPTIONAL clause
  else if (subquery.optional.size() && !subquery.unions.size()) {
    unsigned f = 0, r = 0, fact = 0;
    translateOptional(query,subquery,projection,schema,f,r,fact,1); 
  }
}

static void dumpPostgres(QueryGraph& query,const string& schema)
// Dump a postgres query
{
  cout << "\\timing" << endl;
  cout << "select ";
  {
    unsigned id=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      if (id) cout << ",";
      cout << "s" << id << ".value";
      id++;
    }
  }
  cout << endl;
  cout << "from (" << endl;

  QueryGraph::SubQuery subquery = query.getQuery(); 

  translatePostgres(query,subquery,schema);

  cout << endl << ") facts";
  {
    unsigned id=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      cout << "," << schema <<".strings s" << id;
      id++;
    }
  
  }
  cout << endl;
  cout << "where ";
  {
    unsigned id=0;
    for (QueryGraph::projection_iterator iter=query.projectionBegin(),limit=query.projectionEnd();iter!=limit;++iter) {
      if (id) cout << " and ";
      cout << "s" << id << ".id=facts.r" << id;
      id++;
    }
  }
 
 cout << endl;
 if (query.getLimit() != ~0u)
   cout << "limit " << query.getLimit();

 cout << ";" << endl;
}
//---------------------------------------------------------------------------
int main(int argc,char* argv[])
{
  // Check the arguments
  if (argc<3) {
    cout << "usage: " << argv[0] << " <database> <type> [sparqlfile]" << endl;
    return 1;
  }
  bool monetDB=false;
  if (string(argv[2])=="postgres")
    monetDB=false; else
    if (string(argv[2])=="monetdb")
      monetDB=true; else {
      cout << "unknown method " << argv[2] << endl;
      return 1;
    }

  // Open the database
  Database db;
  if (!db.open(argv[1])) {
    cout << "unable to open database " << argv[1] << endl;
    return 1;
  }

  // Retrieve the query
  string query;
  if (argc>3) {
    ifstream in(argv[3]);
    if (!in.is_open()) {
      cout << "unable to open " << argv[3] << endl;
      return 1;
    }
    query=readInput(in);
  } else {
    query=readInput(cin);
  }

  // Parse it
  QueryGraph queryGraph;
  {
    // Parse the query
    SPARQLLexer lexer(query);
    SPARQLParser parser(lexer);
    try {
      parser.parse();
    } catch (const SPARQLParser::ParserException& e) {
      cout << "parse error: " << e.message << endl;
      return 1;
    }

    // And perform the semantic anaylsis
    SemanticAnalysis semana(db);
    semana.transform(parser,queryGraph);
    if (queryGraph.knownEmpty()) {
      cout << "<empty result>" << endl;
      return 1;
    }
}
    
  // And dump it
  if (monetDB)
    dumpMonetDB(queryGraph); 
  else
    dumpPostgres(queryGraph,databaseName(argv[1]));
}
//---------------------------------------------------------------------------

